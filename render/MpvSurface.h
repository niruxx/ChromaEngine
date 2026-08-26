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
    void setFlip(bool horizontal, bool vertical);
    void setSpeed(double rate);
    void setBrightness(int value);
    void setContrast(int value);
    void setSaturation(int value);
    void setZoom(double logScale);
    void setFrameRateLimit(int fps); // 0 = unlimited

signals:
    void videoSizeChanged(int width, int height);

private slots:
    void pumpEvents();

private:
    void applyVideoFilters();

    mpv_handle* m_mpv = nullptr;
    int m_pendingWidth = 0;
    int m_pendingHeight = 0;
    bool m_flipHorizontal = false;
    bool m_flipVertical = false;
    int m_frameRateLimit = 0;
};

} // namespace colorfy
