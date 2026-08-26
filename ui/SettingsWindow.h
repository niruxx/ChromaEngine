#pragma once

#include <QList>
#include <QMainWindow>
#include <QPixmap>

#include "colorfy/MediaItem.h"
#include "colorfy/MonitorManager.h"

class QLabel;
class QSlider;
class QTimer;
class QCheckBox;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QAction;
class QPushButton;
class QCloseEvent;
class QResizeEvent;

namespace colorfy {

class LibraryItemDelegate;
class SettingsDialog;
class CustomTitleBar;
class AnimatedBackground;

// Wallpaper Engine style library browser: a toolbar (folder / refresh /
// play-pause / settings), a grid of auto-animating preview tiles on the
// left, and a live preview pane plus per-file playback/appearance controls
// and file actions on the right.
class SettingsWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget* parent = nullptr);

    void setMediaItem(const MediaItem& item);

    // Updates which folder the status bar's file count and storage-space
    // readout refer to. setLibraryFiles() alone doesn't know the folder
    // path (just the resulting file list), so callers that switch folders
    // should call this first.
    void setCurrentFolder(const QString& folder);

    // Gates the shared cycling timer that animates library tiles - off by
    // default (see MediaItem::thumbnailAutoPlayEnabled for why).
    void setThumbnailAutoPlayEnabled(bool enabled);

    // Repopulates the grid with these files (absolute paths), re-selecting
    // `selectedPath` if it's among them.
    void setLibraryFiles(const QStringList& filePaths, const QString& selectedPath);

    // Applies a cycling frame sequence to the tile for `filePath`, if it's
    // still in the grid.
    void setFrames(const QString& filePath, const QList<QPixmap>& frames);

    // Native window handle of the live preview surface, for embedding a
    // second mpv instance into it (owned by whoever constructs that surface).
    void* previewNativeHandle() const;

    // Currently previewed file (not necessarily the live desktop wallpaper
    // until "Set as Wallpaper" is used).
    QString previewedPath() const { return m_previewedPath; }

    SettingsDialog* settingsDialog() const { return m_settingsDialog; }

public slots:
    void setVideoResolution(int width, int height);
    void setCloseToTray(bool enabled) { m_closeToTray = enabled; }

signals:
    void folderChanged(const QString& folder);
    void mediaPreviewed(const QString& path);
    void setWallpaperRequested(const QString& path);
    void mutedChanged(bool muted);
    void volumeChanged(int volumePercent);
    void fitModeChanged(FitMode mode);
    void zoomChanged(double logScale);
    void flipChanged(bool horizontal, bool vertical);
    void brightnessChanged(int value);
    void contrastChanged(int value);
    void saturationChanged(int value);
    void speedChanged(double rate);
    void previewFrameRateLimitChanged(int fps);
    void showDesktopIconsChanged(bool show);
    void refreshRequested();
    void playPauseToggled(bool paused);
    void renameRequested(const QString& path, const QString& newName);
    void deleteRequested(const QString& path);

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
#ifdef _WIN32
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private:
    void buildUi();
    QListWidgetItem* findItem(const QString& filePath) const;
    void updateFileInfo(const QString& path);
    void updateStorageLabel();
    void advanceFrame();

    CustomTitleBar* m_titleBar = nullptr;
    AnimatedBackground* m_animatedBackground = nullptr;
    QAction* m_playPauseAction = nullptr;
    QLabel* m_countLabel = nullptr;
    QLabel* m_storageLabel = nullptr;
    QListWidget* m_libraryGrid = nullptr;
    LibraryItemDelegate* m_delegate = nullptr;
    QWidget* m_previewWidget = nullptr;
    QLabel* m_previewFilenameLabel = nullptr;
    QLabel* m_fileSizeLabel = nullptr;
    QLabel* m_fileResolutionLabel = nullptr;
    QPushButton* m_renameButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_setWallpaperButton = nullptr;
    QCheckBox* m_muteCheckBox = nullptr;
    QSlider* m_volumeSlider = nullptr;
    QComboBox* m_fitModeCombo = nullptr;
    QWidget* m_zoomRow = nullptr;
    QSlider* m_zoomSlider = nullptr;
    QLabel* m_zoomLabel = nullptr;
    QCheckBox* m_flipHCheckBox = nullptr;
    QCheckBox* m_flipVCheckBox = nullptr;
    QCheckBox* m_showDesktopIconsCheckBox = nullptr;
    QSlider* m_brightnessSlider = nullptr;
    QSlider* m_contrastSlider = nullptr;
    QSlider* m_saturationSlider = nullptr;
    QSlider* m_speedSlider = nullptr;
    QLabel* m_speedLabel = nullptr;
    QComboBox* m_frameRateLimitCombo = nullptr;

    SettingsDialog* m_settingsDialog = nullptr;

    QString m_currentFolder;
    QString m_previewedPath;
    bool m_closeToTray = true;
    int m_currentFrame = 0;
    QTimer* m_frameTimer = nullptr;
};

} // namespace colorfy
