#include "DesktopOverlayWindow.h"
#include "MemoryLimiter.h"
#include "StartupManager.h"
#include "ThumbnailGenerator.h"
#include "WallpaperWindow.h"
#include "WorkerWHost.h"

#include "render/MpvSurface.h"
#include "ui/IconFactory.h"
#include "ui/SettingsDialog.h"
#include "ui/SettingsWindow.h"
#include "ui/TrayIcon.h"

#include "colorfy/ConfigStore.h"
#include "colorfy/LibraryScanner.h"
#include "colorfy/MonitorManager.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QSharedMemory>
#include <QTimer>

#include <windows.h>

#include <functional>
#include <memory>

using namespace colorfy;

namespace {

struct MonitorSurface {
    WallpaperWindow* window = nullptr;
    MpvSurface* mpv = nullptr;
    DesktopOverlayWindow* overlay = nullptr;
};

// Fit/flip/filters/speed/zoom - everything except mute/volume, which the
// live preview intentionally never picks up (it always stays muted so
// previewing a file never doubles up audio with the live wallpaper).
void applyAppearance(MpvSurface* surface, const MediaItem& media)
{
    surface->setFitMode(media.fitMode);
    surface->setFlip(media.flipHorizontal, media.flipVertical);
    surface->setBrightness(media.brightness);
    surface->setContrast(media.contrast);
    surface->setSaturation(media.saturation);
    surface->setSpeed(media.playbackRate);
    surface->setZoom(media.zoom);
}

void loadFolderIntoSettings(SettingsWindow* settingsWindow, ThumbnailGenerator* thumbnailGenerator,
                             const std::shared_ptr<MediaItem>& media)
{
    settingsWindow->setCurrentFolder(media->libraryFolder);
    const QStringList files = LibraryScanner::scan(media->libraryFolder);
    settingsWindow->setLibraryFiles(files, media->filePath);

    for (const QString& path : files) {
        if (!LibraryScanner::isGif(path))
            thumbnailGenerator->enqueue(path);
    }
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName(QStringLiteral("ChromaEngine"));
    app.setOrganizationName(QStringLiteral("chroma-engine"));
    app.setWindowIcon(IconFactory::appLogo(64));

    // Single-instance guard: a second launch just notifies and exits.
    QSharedMemory singleInstanceGuard(QStringLiteral("chroma-engine-single-instance"));
    if (!singleInstanceGuard.create(1)) {
        QMessageBox::information(nullptr, QStringLiteral("ChromaEngine"),
                                  QStringLiteral("ChromaEngine is already running."));
        return 0;
    }

    auto media = std::make_shared<MediaItem>(ConfigStore::load());

    if (media->memoryLimitEnabled)
        MemoryLimiter::apply(media->memoryLimitMb);

    // Resolve the ambiguous "empty enabledMonitorIds" default into an
    // explicit primary-only list on first run, so later toggle logic
    // (append/removeAll) never has to guess what "empty" currently means.
    if (media->enabledMonitorIds.isEmpty()) {
        for (const MonitorInfo& info : MonitorManager::listMonitors()) {
            if (info.isPrimary) {
                media->enabledMonitorIds.append(info.id);
                break;
            }
        }
        ConfigStore::save(*media);
    }

    // Debounced persistence: sliders (brightness/contrast/saturation/volume/
    // zoom/speed) fire valueChanged continuously while being dragged, and
    // saving synchronously on every tick meant a full JSON serialize + disk
    // write per pixel of drag movement - a real source of UI stutter.
    // Settings are applied to the in-memory MediaItem (and to mpv) instantly
    // either way; only the on-disk write is delayed and coalesced.
    auto* configSaveTimer = new QTimer(&app);
    configSaveTimer->setSingleShot(true);
    configSaveTimer->setInterval(400);
    QObject::connect(configSaveTimer, &QTimer::timeout, &app, [media] { ConfigStore::save(*media); });
    auto scheduleConfigSave = [configSaveTimer] { configSaveTimer->start(); };
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [configSaveTimer, media] {
        if (configSaveTimer->isActive()) {
            configSaveTimer->stop();
            ConfigStore::save(*media);
        }
    });

    auto* workerWHost = new WorkerWHost(&app);
    auto currentWorkerW = std::make_shared<void*>(workerWHost->attach());

    auto monitorSurfaces = std::make_shared<QList<MonitorSurface>>();

    auto* thumbnailGenerator = new ThumbnailGenerator(&app);
    auto* settingsWindow = new SettingsWindow();

    std::function<void()> rebuildSurfaces = [&app, workerWHost, currentWorkerW, monitorSurfaces, media]() {
        for (const MonitorSurface& s : *monitorSurfaces) {
            delete s.mpv;
            delete s.window;
            delete s.overlay;
        }
        monitorSurfaces->clear();

        for (const MonitorInfo& info : MonitorManager::listMonitors()) {
            if (!media->enabledMonitorIds.contains(info.id))
                continue;

            auto* window = new WallpaperWindow(info.geometry);
            window->show();
            window->attachToWorkerW(*currentWorkerW);
            window->setShowDesktopIcons(media->showDesktopIcons);

            auto* surface = new MpvSurface(window->nativeHandle(), &app);
            applyAppearance(surface, *media);
            surface->setMuted(media->muted);
            surface->setVolume(media->volume);
            if (!media->filePath.isEmpty())
                surface->loadFile(media->filePath);

            // Not reparented into WorkerW (see DesktopOverlayWindow's class
            // comment) - a separate, genuinely top-level window instead.
            auto* overlay = new DesktopOverlayWindow(info.geometry);
            overlay->applySettings(*media);
            overlay->showOnDesktop();

            monitorSurfaces->append({window, surface, overlay});
        }
    };
    rebuildSurfaces();
    workerWHost->setShowDesktopIcons(media->showDesktopIcons);

    workerWHost->startWatchdog();
    QObject::connect(workerWHost, &WorkerWHost::workerWChanged, &app, [currentWorkerW, monitorSurfaces](void* hwnd) {
        *currentWorkerW = hwnd;
        for (const MonitorSurface& s : *monitorSurfaces)
            s.window->attachToWorkerW(hwnd);
    });

    // Applies a change to the live desktop wallpaper(s) and (appearance-only
    // changes) the preview surface, declared after previewSurface below.
    auto forEachDesktopSurface = [monitorSurfaces](const std::function<void(MpvSurface*)>& fn) {
        for (const MonitorSurface& s : *monitorSurfaces)
            fn(s.mpv);
    };

    // Live preview pane: a second, always-muted mpv instance embedded in the
    // settings window, independent of the actual desktop wallpaper surface.
    auto* previewSurface = new MpvSurface(settingsWindow->previewNativeHandle(), &app);
    previewSurface->setMuted(true);
    applyAppearance(previewSurface, *media);
    previewSurface->setFrameRateLimit(media->previewFrameRateLimit);
    if (!media->filePath.isEmpty())
        previewSurface->loadFile(media->filePath);
    QObject::connect(previewSurface, &MpvSurface::videoSizeChanged, settingsWindow, &SettingsWindow::setVideoResolution);

    auto forEachSurface = [previewSurface, forEachDesktopSurface](const std::function<void(MpvSurface*)>& fn) {
        fn(previewSurface);
        forEachDesktopSurface(fn);
    };

    settingsWindow->setMediaItem(*media);
    settingsWindow->setThumbnailAutoPlayEnabled(media->thumbnailAutoPlayEnabled);
    thumbnailGenerator->setAutoPlayEnabled(media->thumbnailAutoPlayEnabled);

    QObject::connect(thumbnailGenerator, &ThumbnailGenerator::framesReady, settingsWindow, &SettingsWindow::setFrames);

    if (!media->libraryFolder.isEmpty())
        loadFolderIntoSettings(settingsWindow, thumbnailGenerator, media);

    // Foreground by default on launch - only stay in the tray if the user
    // has explicitly opted into that via "Start minimized to the tray".
    if (!media->startMinimized) {
        settingsWindow->show();
        settingsWindow->raise();
        settingsWindow->activateWindow();
    }

    auto* trayIcon = new TrayIcon(&app);

    QObject::connect(trayIcon, &TrayIcon::openSettingsRequested, settingsWindow, [settingsWindow] {
        settingsWindow->show();
        settingsWindow->raise();
        settingsWindow->activateWindow();
    });
    QObject::connect(trayIcon, &TrayIcon::quitRequested, &app, &QApplication::quit);
    QObject::connect(trayIcon, &TrayIcon::pauseToggled, &app,
                      [forEachDesktopSurface](bool paused) { forEachDesktopSurface([paused](MpvSurface* s) { s->setPaused(paused); }); });

    auto applyLibraryFolder = [settingsWindow, thumbnailGenerator, media, scheduleConfigSave](const QString& folder) {
        media->libraryFolder = folder;
        scheduleConfigSave();
        loadFolderIntoSettings(settingsWindow, thumbnailGenerator, media);
    };
    QObject::connect(settingsWindow, &SettingsWindow::folderChanged, &app, applyLibraryFolder);
    QObject::connect(settingsWindow, &SettingsWindow::mediaPreviewed, &app,
                      [previewSurface](const QString& path) { previewSurface->loadFile(path); });
    QObject::connect(settingsWindow, &SettingsWindow::setWallpaperRequested, &app,
                      [forEachDesktopSurface, media, scheduleConfigSave](const QString& path) {
                          media->filePath = path;
                          scheduleConfigSave();
                          forEachDesktopSurface([path](MpvSurface* s) { s->loadFile(path); });
                      });
    QObject::connect(settingsWindow, &SettingsWindow::refreshRequested, &app,
                      [settingsWindow, thumbnailGenerator, media] {
                          loadFolderIntoSettings(settingsWindow, thumbnailGenerator, media);
                      });
    QObject::connect(settingsWindow, &SettingsWindow::playPauseToggled, &app, [forEachDesktopSurface](bool paused) {
        forEachDesktopSurface([paused](MpvSurface* s) { s->setPaused(paused); });
    });
    QObject::connect(settingsWindow, &SettingsWindow::mutedChanged, &app,
                      [forEachDesktopSurface, media, scheduleConfigSave](bool muted) {
                          media->muted = muted;
                          scheduleConfigSave();
                          forEachDesktopSurface([muted](MpvSurface* s) { s->setMuted(muted); });
                      });
    QObject::connect(settingsWindow, &SettingsWindow::volumeChanged, &app,
                      [forEachDesktopSurface, media, scheduleConfigSave](int volume) {
                          media->volume = volume;
                          scheduleConfigSave();
                          forEachDesktopSurface([volume](MpvSurface* s) { s->setVolume(volume); });
                      });
    QObject::connect(settingsWindow, &SettingsWindow::fitModeChanged, &app,
                      [forEachSurface, media, scheduleConfigSave](FitMode mode) {
                          media->fitMode = mode;
                          scheduleConfigSave();
                          forEachSurface([mode](MpvSurface* s) { s->setFitMode(mode); });
                      });
    QObject::connect(settingsWindow, &SettingsWindow::zoomChanged, &app,
                      [forEachSurface, media, scheduleConfigSave](double zoom) {
                          media->zoom = zoom;
                          scheduleConfigSave();
                          forEachSurface([zoom](MpvSurface* s) { s->setZoom(zoom); });
                      });
    QObject::connect(settingsWindow, &SettingsWindow::flipChanged, &app,
                      [forEachSurface, media, scheduleConfigSave](bool h, bool v) {
                          media->flipHorizontal = h;
                          media->flipVertical = v;
                          scheduleConfigSave();
                          forEachSurface([h, v](MpvSurface* s) { s->setFlip(h, v); });
                      });
    QObject::connect(settingsWindow, &SettingsWindow::brightnessChanged, &app,
                      [forEachSurface, media, scheduleConfigSave](int value) {
                          media->brightness = value;
                          scheduleConfigSave();
                          forEachSurface([value](MpvSurface* s) { s->setBrightness(value); });
                      });
    QObject::connect(settingsWindow, &SettingsWindow::contrastChanged, &app,
                      [forEachSurface, media, scheduleConfigSave](int value) {
                          media->contrast = value;
                          scheduleConfigSave();
                          forEachSurface([value](MpvSurface* s) { s->setContrast(value); });
                      });
    QObject::connect(settingsWindow, &SettingsWindow::saturationChanged, &app,
                      [forEachSurface, media, scheduleConfigSave](int value) {
                          media->saturation = value;
                          scheduleConfigSave();
                          forEachSurface([value](MpvSurface* s) { s->setSaturation(value); });
                      });
    // Preview-only: unlike the other appearance settings, the frame rate
    // limit is a tool for smoothly scrubbing/previewing without spinning up
    // the GPU, so it deliberately never touches the live desktop wallpaper.
    QObject::connect(settingsWindow, &SettingsWindow::previewFrameRateLimitChanged, &app,
                      [previewSurface, media, scheduleConfigSave](int fps) {
                          media->previewFrameRateLimit = fps;
                          scheduleConfigSave();
                          previewSurface->setFrameRateLimit(fps);
                      });
    QObject::connect(settingsWindow, &SettingsWindow::speedChanged, &app,
                      [forEachSurface, media, scheduleConfigSave](double rate) {
                          media->playbackRate = rate;
                          scheduleConfigSave();
                          forEachSurface([rate](MpvSurface* s) { s->setSpeed(rate); });
                      });
    QObject::connect(settingsWindow, &SettingsWindow::showDesktopIconsChanged, &app,
                      [workerWHost, monitorSurfaces, media, scheduleConfigSave](bool show) {
                          media->showDesktopIcons = show;
                          scheduleConfigSave();
                          workerWHost->setShowDesktopIcons(show);
                          for (const MonitorSurface& s : *monitorSurfaces)
                              s.window->setShowDesktopIcons(show);
                      });

    QObject::connect(settingsWindow, &SettingsWindow::renameRequested, &app,
                      [settingsWindow, thumbnailGenerator, media](const QString& path, const QString& newName) {
                          const QFileInfo info(path);
                          const QString newPath = info.absoluteDir().filePath(newName);
                          if (!QFile::rename(path, newPath)) {
                              QMessageBox::warning(settingsWindow, QStringLiteral("Rename failed"),
                                                    QStringLiteral("Could not rename the file."));
                              return;
                          }
                          if (media->filePath == path) {
                              media->filePath = newPath;
                              ConfigStore::save(*media);
                          }
                          loadFolderIntoSettings(settingsWindow, thumbnailGenerator, media);
                      });
    QObject::connect(settingsWindow, &SettingsWindow::deleteRequested, &app,
                      [settingsWindow, thumbnailGenerator, media](const QString& path) {
                          if (!QFile::remove(path)) {
                              QMessageBox::warning(settingsWindow, QStringLiteral("Delete failed"),
                                                    QStringLiteral("Could not delete the file."));
                              return;
                          }
                          if (media->filePath == path) {
                              media->filePath.clear();
                              ConfigStore::save(*media);
                          }
                          loadFolderIntoSettings(settingsWindow, thumbnailGenerator, media);
                      });

    // --- Preferences dialog wiring ---
    SettingsDialog* dialog = settingsWindow->settingsDialog();
    dialog->setLaunchAtStartup(media->launchAtStartup);
    dialog->setCloseToTray(media->closeToTray);
    dialog->setStartMinimized(media->startMinimized);
    dialog->setThumbnailAutoPlay(media->thumbnailAutoPlayEnabled);
    dialog->setMemoryLimit(media->memoryLimitEnabled, media->memoryLimitMb);
    dialog->setDefaultFolder(media->libraryFolder);
    dialog->setBackgroundTheme(media->backgroundTheme);
    dialog->setMonitors(MonitorManager::listMonitors(), media->enabledMonitorIds);
    dialog->setClockSettings(media->clockEnabled, static_cast<int>(media->clockLayout),
                              static_cast<int>(media->clockTheme), media->clockFontFamily, media->clockFontSize,
                              media->clockRotation, static_cast<int>(media->clockPosition), media->clockMargin);
    dialog->setCalendarSettings(media->calendarEnabled, static_cast<int>(media->calendarTheme),
                                 static_cast<int>(media->calendarPosition), media->calendarMargin);
    dialog->setBatterySettings(media->batteryIndicatorEnabled, static_cast<int>(media->batteryTheme),
                                static_cast<int>(media->batteryPosition), media->batteryMargin);

    auto forEachOverlay = [monitorSurfaces](const std::function<void(DesktopOverlayWindow*)>& fn) {
        for (const MonitorSurface& s : *monitorSurfaces)
            fn(s.overlay);
    };

    QObject::connect(dialog, &SettingsDialog::clockSettingsChanged, &app,
                      [media, forEachOverlay, scheduleConfigSave](bool enabled, int layout, int theme,
                                                                   const QString& fontFamily, int fontSize,
                                                                   int rotation, int position, int margin) {
                          media->clockEnabled = enabled;
                          media->clockLayout = static_cast<ClockLayout>(layout);
                          media->clockTheme = static_cast<OverlayTheme>(theme);
                          media->clockFontFamily = fontFamily;
                          media->clockFontSize = fontSize;
                          media->clockRotation = rotation;
                          media->clockPosition = static_cast<OverlayPosition>(position);
                          media->clockMargin = margin;
                          scheduleConfigSave();
                          forEachOverlay([media](DesktopOverlayWindow* o) { o->applySettings(*media); });
                      });
    QObject::connect(dialog, &SettingsDialog::calendarSettingsChanged, &app,
                      [media, forEachOverlay, scheduleConfigSave](bool enabled, int theme, int position, int margin) {
                          media->calendarEnabled = enabled;
                          media->calendarTheme = static_cast<OverlayTheme>(theme);
                          media->calendarPosition = static_cast<OverlayPosition>(position);
                          media->calendarMargin = margin;
                          scheduleConfigSave();
                          forEachOverlay([media](DesktopOverlayWindow* o) { o->applySettings(*media); });
                      });
    QObject::connect(dialog, &SettingsDialog::batterySettingsChanged, &app,
                      [media, forEachOverlay, scheduleConfigSave](bool enabled, int theme, int position, int margin) {
                          media->batteryIndicatorEnabled = enabled;
                          media->batteryTheme = static_cast<OverlayTheme>(theme);
                          media->batteryPosition = static_cast<OverlayPosition>(position);
                          media->batteryMargin = margin;
                          scheduleConfigSave();
                          forEachOverlay([media](DesktopOverlayWindow* o) { o->applySettings(*media); });
                      });

    QObject::connect(dialog, &SettingsDialog::defaultFolderChanged, &app, applyLibraryFolder);
    QObject::connect(dialog, &SettingsDialog::backgroundThemeChanged, &app, [media, scheduleConfigSave](int theme) {
        media->backgroundTheme = theme;
        scheduleConfigSave();
    });

    QObject::connect(dialog, &SettingsDialog::launchAtStartupToggled, &app, [media](bool enabled) {
        media->launchAtStartup = enabled;
        ConfigStore::save(*media);
        StartupManager::setEnabled(enabled);
    });
    QObject::connect(dialog, &SettingsDialog::closeToTrayToggled, &app, [media, settingsWindow](bool enabled) {
        media->closeToTray = enabled;
        ConfigStore::save(*media);
        settingsWindow->setCloseToTray(enabled);
    });
    QObject::connect(dialog, &SettingsDialog::startMinimizedToggled, &app, [media](bool enabled) {
        media->startMinimized = enabled;
        ConfigStore::save(*media);
    });
    QObject::connect(dialog, &SettingsDialog::thumbnailAutoPlayToggled, &app,
                      [media, settingsWindow, thumbnailGenerator](bool enabled) {
                          media->thumbnailAutoPlayEnabled = enabled;
                          ConfigStore::save(*media);
                          settingsWindow->setThumbnailAutoPlayEnabled(enabled);
                          thumbnailGenerator->setAutoPlayEnabled(enabled);
                      });
    QObject::connect(dialog, &SettingsDialog::memoryLimitChanged, &app,
                      [media, scheduleConfigSave](bool enabled, int limitMb) {
                          media->memoryLimitEnabled = enabled;
                          media->memoryLimitMb = limitMb;
                          scheduleConfigSave();
                      });
    QObject::connect(dialog, &SettingsDialog::monitorToggled, &app,
                      [media, rebuildSurfaces](const QString& id, bool enabled) {
                          if (enabled) {
                              if (!media->enabledMonitorIds.contains(id))
                                  media->enabledMonitorIds.append(id);
                          } else {
                              media->enabledMonitorIds.removeAll(id);
                          }
                          ConfigStore::save(*media);
                          rebuildSurfaces();
                      });

    return app.exec();
}
