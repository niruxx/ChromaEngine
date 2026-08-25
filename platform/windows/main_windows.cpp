#include "ThumbnailGenerator.h"
#include "WallpaperWindow.h"
#include "WorkerWHost.h"

#include "render/MpvSurface.h"
#include "ui/SettingsWindow.h"
#include "ui/TrayIcon.h"

#include "colorfy/ConfigStore.h"
#include "colorfy/LibraryScanner.h"
#include "colorfy/MonitorManager.h"

#include <QApplication>
#include <QMessageBox>
#include <QSharedMemory>

#include <memory>

using namespace colorfy;

namespace {

void loadFolderIntoSettings(SettingsWindow* settingsWindow, ThumbnailGenerator* thumbnailGenerator,
                             const std::shared_ptr<MediaItem>& media)
{
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
    app.setApplicationName(QStringLiteral("ColorfyEngine"));
    app.setOrganizationName(QStringLiteral("colorfy-engine"));

    // Single-instance guard: a second launch just notifies and exits.
    QSharedMemory singleInstanceGuard(QStringLiteral("colorfy-engine-single-instance"));
    if (!singleInstanceGuard.create(1)) {
        QMessageBox::information(nullptr, QStringLiteral("Colorfy Engine"),
                                  QStringLiteral("Colorfy Engine is already running."));
        return 0;
    }

    auto media = std::make_shared<MediaItem>(ConfigStore::load());

    const QRect geometry = MonitorManager::primaryGeometry();
    auto* wallpaperWindow = new WallpaperWindow(geometry);
    wallpaperWindow->show();

    auto* workerWHost = new WorkerWHost(&app);
    void* workerW = workerWHost->attach();
    wallpaperWindow->attachToWorkerW(workerW);
    workerWHost->startWatchdog();

    QObject::connect(workerWHost, &WorkerWHost::workerWChanged, wallpaperWindow,
                      [wallpaperWindow](void* hwnd) { wallpaperWindow->attachToWorkerW(hwnd); });

    auto* mpvSurface = new MpvSurface(wallpaperWindow->nativeHandle(), &app);
    mpvSurface->setFitMode(media->fitMode);
    mpvSurface->setMuted(media->muted);
    mpvSurface->setVolume(media->volume);
    if (!media->filePath.isEmpty())
        mpvSurface->loadFile(media->filePath);

    auto* thumbnailGenerator = new ThumbnailGenerator(&app);

    auto* settingsWindow = new SettingsWindow();
    settingsWindow->setMediaItem(*media);

    // Live preview pane: a second, always-muted mpv instance embedded in the
    // settings window, independent of the actual desktop wallpaper surface.
    auto* previewSurface = new MpvSurface(settingsWindow->previewNativeHandle(), &app);
    previewSurface->setMuted(true);
    if (!media->filePath.isEmpty())
        previewSurface->loadFile(media->filePath);

    QObject::connect(thumbnailGenerator, &ThumbnailGenerator::thumbnailReady, settingsWindow,
                      &SettingsWindow::setThumbnail);

    if (!media->libraryFolder.isEmpty())
        loadFolderIntoSettings(settingsWindow, thumbnailGenerator, media);

    // First run (nothing configured yet): open settings immediately instead
    // of leaving the user with just a black background and a tray icon.
    if (media->libraryFolder.isEmpty()) {
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

    QObject::connect(settingsWindow, &SettingsWindow::folderChanged, &app,
                      [settingsWindow, thumbnailGenerator, media](const QString& folder) {
                          media->libraryFolder = folder;
                          ConfigStore::save(*media);
                          loadFolderIntoSettings(settingsWindow, thumbnailGenerator, media);
                      });
    QObject::connect(settingsWindow, &SettingsWindow::mediaSelected, &app,
                      [mpvSurface, previewSurface, media](const QString& path) {
                          media->filePath = path;
                          mpvSurface->loadFile(path);
                          previewSurface->loadFile(path);
                          ConfigStore::save(*media);
                      });
    QObject::connect(settingsWindow, &SettingsWindow::refreshRequested, &app,
                      [settingsWindow, thumbnailGenerator, media] {
                          loadFolderIntoSettings(settingsWindow, thumbnailGenerator, media);
                      });
    QObject::connect(settingsWindow, &SettingsWindow::playPauseToggled, &app, [mpvSurface](bool paused) {
        mpvSurface->setPaused(paused);
    });
    QObject::connect(settingsWindow, &SettingsWindow::mutedChanged, &app, [mpvSurface, media](bool muted) {
        media->muted = muted;
        mpvSurface->setMuted(muted);
        ConfigStore::save(*media);
    });
    QObject::connect(settingsWindow, &SettingsWindow::volumeChanged, &app, [mpvSurface, media](int volume) {
        media->volume = volume;
        mpvSurface->setVolume(volume);
        ConfigStore::save(*media);
    });
    QObject::connect(settingsWindow, &SettingsWindow::fitModeChanged, &app, [mpvSurface, media](FitMode mode) {
        media->fitMode = mode;
        mpvSurface->setFitMode(mode);
        ConfigStore::save(*media);
    });

    return app.exec();
}
