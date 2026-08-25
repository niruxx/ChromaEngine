#pragma once

#include <QObject>
#include <QString>

#include "colorfy/MediaItem.h"

struct mpv_handle;

namespace colorfy {

// Renders local video/GIF files via libmpv, embedded into a native window
// handle (HWND on Windows) supplied by the platform layer. MP4 and GIF go
// through the same decode/render path, so there is no separate GIF codepath.
class MpvSurface : public QObject {
    Q_OBJECT
public:
    explicit MpvSurface(void* nativeWindowHandle, QObject* parent = nullptr);
    ~MpvSurface() override;

    void loadFile(const QString& path);
    void setMuted(bool muted);
    void setVolume(int volumePercent);
    void setFitMode(FitMode mode);
    void setPaused(bool paused);

private slots:
    void pumpEvents();

private:
    mpv_handle* m_mpv = nullptr;
};

} // namespace colorfy
