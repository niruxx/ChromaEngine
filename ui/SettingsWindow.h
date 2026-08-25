#pragma once

#include <QMainWindow>
#include <QPixmap>

#include "colorfy/MediaItem.h"

class QLabel;
class QSlider;
class QCheckBox;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QAction;

namespace colorfy {

// Wallpaper Engine style library browser: a toolbar (folder / refresh /
// play-pause), a grid of clickable preview tiles on the left, and a live
// preview pane (an embedded, always-muted mpv surface) plus playback
// controls on the right.
class SettingsWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget* parent = nullptr);

    void setMediaItem(const MediaItem& item);

    // Repopulates the grid with these files (absolute paths), re-selecting
    // `selectedPath` if it's among them.
    void setLibraryFiles(const QStringList& filePaths, const QString& selectedPath);

    // Applies a thumbnail to the tile for `filePath`, if it's still in the grid.
    void setThumbnail(const QString& filePath, const QPixmap& pixmap);

    // Native window handle of the live preview surface, for embedding a
    // second mpv instance into it (owned by whoever constructs that surface).
    void* previewNativeHandle() const;

signals:
    void folderChanged(const QString& folder);
    void mediaSelected(const QString& path);
    void mutedChanged(bool muted);
    void volumeChanged(int volumePercent);
    void fitModeChanged(FitMode mode);
    void refreshRequested();
    void playPauseToggled(bool paused);

private:
    void buildUi();
    QListWidgetItem* findItem(const QString& filePath) const;

    QAction* m_playPauseAction = nullptr;
    QLabel* m_countLabel = nullptr;
    QListWidget* m_libraryGrid = nullptr;
    QWidget* m_previewWidget = nullptr;
    QLabel* m_previewFilenameLabel = nullptr;
    QCheckBox* m_muteCheckBox = nullptr;
    QSlider* m_volumeSlider = nullptr;
    QComboBox* m_fitModeCombo = nullptr;

    QString m_currentFolder;
};

} // namespace colorfy
