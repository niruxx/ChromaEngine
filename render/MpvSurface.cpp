#include "MpvSurface.h"

#include <QDebug>
#include <QTimer>

#include <cstdint>

#include <mpv/client.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace colorfy {

namespace {

void checkError(int status, const char* what)
{
    if (status < 0)
        qWarning() << "mpv error in" << what << ":" << mpv_error_string(status);
}

} // namespace

MpvSurface::MpvSurface(void* nativeWindowHandle, QObject* parent)
    : QObject(parent)
{
    m_mpv = mpv_create();
    if (!m_mpv) {
        qWarning() << "Failed to create mpv instance";
        return;
    }

#ifdef _WIN32
    int64_t wid = reinterpret_cast<int64_t>(reinterpret_cast<HWND>(nativeWindowHandle));
    mpv_set_option(m_mpv, "wid", MPV_FORMAT_INT64, &wid);
#endif

    // Wallpaper defaults: loop forever, no on-screen controls, no input
    // handling (the window sits behind desktop icons, it never has focus).
    mpv_set_option_string(m_mpv, "loop-file", "inf");
    mpv_set_option_string(m_mpv, "keep-open", "yes");
    mpv_set_option_string(m_mpv, "mute", "yes");
    mpv_set_option_string(m_mpv, "hwdec", "auto");
    // Bounds the software-decode fallback's thread pool (FFmpeg otherwise
    // defaults to roughly one thread per CPU core) - hwdec=auto normally
    // avoids needing this at all, but if hardware decode isn't available
    // for a given file, an uncapped software decoder here plus this app's
    // other concurrent mpv instances (there can be several: one wallpaper
    // surface per enabled monitor, the preview pane, and a throwaway one
    // for thumbnail generation) can add up to enough decode threads to
    // starve the main thread and make the whole app feel unresponsive.
    mpv_set_option_string(m_mpv, "vd-lavc-threads", "4");
    mpv_set_option_string(m_mpv, "keepaspect", "yes");
    mpv_set_option_string(m_mpv, "panscan", "1.0");
    mpv_set_option_string(m_mpv, "osc", "no");
    mpv_set_option_string(m_mpv, "osd-level", "0");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    mpv_set_option_string(m_mpv, "cursor-autohide", "no");

    checkError(mpv_initialize(m_mpv), "mpv_initialize");

    mpv_observe_property(m_mpv, 0, "width", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpv, 0, "height", MPV_FORMAT_INT64);

    // Drain mpv's event queue periodically so it never backs up. Playback
    // itself happens on mpv's internal render thread via the wid handle.
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MpvSurface::pumpEvents);
    timer->start(50);
}

MpvSurface::~MpvSurface()
{
    if (m_mpv)
        mpv_terminate_destroy(m_mpv);
}

void MpvSurface::loadFile(const QString& path)
{
    if (!m_mpv || path.isEmpty())
        return;

    const QByteArray utf8Path = path.toUtf8();
    const char* args[] = {"loadfile", utf8Path.constData(), nullptr};
    checkError(mpv_command(m_mpv, args), "loadfile");
}

void MpvSurface::setMuted(bool muted)
{
    if (!m_mpv)
        return;
    mpv_set_property_string(m_mpv, "mute", muted ? "yes" : "no");
}

void MpvSurface::setPaused(bool paused)
{
    if (!m_mpv)
        return;
    int flag = paused ? 1 : 0;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag);
}

void MpvSurface::setVolume(int volumePercent)
{
    if (!m_mpv)
        return;
    double vol = qBound(0, volumePercent, 100);
    mpv_set_property(m_mpv, "volume", MPV_FORMAT_DOUBLE, &vol);
}

void MpvSurface::setFitMode(FitMode mode)
{
    if (!m_mpv)
        return;

    // mpv_set_property_string, not mpv_set_option_string: per mpv's own
    // client API docs, mpv_set_option_string is meant for pre-playback
    // configuration and isn't guaranteed to propagate to an already-
    // running instance - which is always the case here, since this is
    // called to change alignment on a file that's already loaded and
    // playing (the wallpaper loads once and plays forever; the preview
    // loads once per selection). mpv_set_property_string is the documented
    // way to change a running option live. This was the whole reason
    // alignment appeared to silently do nothing.
    switch (mode) {
    case FitMode::Fill:
        mpv_set_property_string(m_mpv, "keepaspect", "yes");
        mpv_set_property_string(m_mpv, "panscan", "1.0");
        mpv_set_property_string(m_mpv, "video-unscaled", "no");
        break;
    case FitMode::Fit:
        mpv_set_property_string(m_mpv, "keepaspect", "yes");
        mpv_set_property_string(m_mpv, "panscan", "0.0");
        mpv_set_property_string(m_mpv, "video-unscaled", "no");
        break;
    case FitMode::Stretch:
        mpv_set_property_string(m_mpv, "keepaspect", "no");
        mpv_set_property_string(m_mpv, "video-unscaled", "no");
        break;
    case FitMode::Center:
        mpv_set_property_string(m_mpv, "keepaspect", "yes");
        mpv_set_property_string(m_mpv, "panscan", "0.0");
        mpv_set_property_string(m_mpv, "video-unscaled", "yes");
        break;
    case FitMode::Free:
        mpv_set_property_string(m_mpv, "keepaspect", "yes");
        mpv_set_property_string(m_mpv, "panscan", "0.0");
        mpv_set_property_string(m_mpv, "video-unscaled", "no");
        break;
    }
}

void MpvSurface::setFlip(bool horizontal, bool vertical)
{
    m_flipHorizontal = horizontal;
    m_flipVertical = vertical;
    applyVideoFilters();
}

void MpvSurface::setFrameRateLimit(int fps)
{
    m_frameRateLimit = qMax(0, fps);
    applyVideoFilters();
}

void MpvSurface::applyVideoFilters()
{
    if (!m_mpv)
        return;

    // hwdec=auto decodes into GPU-resident surfaces (e.g. d3d11) that
    // hflip/vflip - plain software filters - can't operate on directly:
    // mpv's filter graph fails to configure and it silently disables the
    // filter ("Disabling filter hflip.00 because it has failed") - visible
    // only in mpv's own log messages, not in mpv_set_property_string's
    // return value, which still reports success. Bridging the gap with an
    // explicit hwdownload hit a separate bug in this mpv/FFmpeg build's
    // auto-negotiation (it picks an unusable 1-bit "monow" format no matter
    // what format is pinned immediately after it), so instead: fall back to
    // software decode while a flip is active, sidestepping the hw-frame
    // pipeline entirely. mpv re-initializes the decoder for this on the
    // next keyframe without needing a full file reload.
    const bool needsSoftwareDecode = m_flipHorizontal || m_flipVertical;
    mpv_set_property_string(m_mpv, "hwdec", needsSoftwareDecode ? "no" : "auto");

    QStringList filters;
    if (m_flipHorizontal)
        filters << QStringLiteral("hflip");
    if (m_flipVertical)
        filters << QStringLiteral("vflip");
    if (m_frameRateLimit > 0)
        filters << QStringLiteral("fps=%1").arg(m_frameRateLimit);

    const QByteArray vf = filters.join(QLatin1Char(',')).toUtf8();
    mpv_set_property_string(m_mpv, "vf", vf.constData());
}

void MpvSurface::setSpeed(double rate)
{
    if (!m_mpv)
        return;
    double clamped = qBound(0.25, rate, 3.0);
    mpv_set_property(m_mpv, "speed", MPV_FORMAT_DOUBLE, &clamped);
}

void MpvSurface::setBrightness(int value)
{
    if (!m_mpv)
        return;
    int64_t clamped = qBound(-100, value, 100);
    mpv_set_property(m_mpv, "brightness", MPV_FORMAT_INT64, &clamped);
}

void MpvSurface::setContrast(int value)
{
    if (!m_mpv)
        return;
    int64_t clamped = qBound(-100, value, 100);
    mpv_set_property(m_mpv, "contrast", MPV_FORMAT_INT64, &clamped);
}

void MpvSurface::setSaturation(int value)
{
    if (!m_mpv)
        return;
    int64_t clamped = qBound(-100, value, 100);
    mpv_set_property(m_mpv, "saturation", MPV_FORMAT_INT64, &clamped);
}

void MpvSurface::setZoom(double logScale)
{
    if (!m_mpv)
        return;
    mpv_set_property(m_mpv, "video-zoom", MPV_FORMAT_DOUBLE, &logScale);
}

void MpvSurface::pumpEvents()
{
    if (!m_mpv)
        return;

    while (true) {
        mpv_event* event = mpv_wait_event(m_mpv, 0);
        if (!event || event->event_id == MPV_EVENT_NONE)
            break;

        if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            auto* prop = static_cast<mpv_event_property*>(event->data);
            if (!prop || !prop->data || prop->format != MPV_FORMAT_INT64)
                continue;

            const int64_t value = *static_cast<int64_t*>(prop->data);
            if (qstrcmp(prop->name, "width") == 0)
                m_pendingWidth = static_cast<int>(value);
            else if (qstrcmp(prop->name, "height") == 0)
                m_pendingHeight = static_cast<int>(value);

            if (m_pendingWidth > 0 && m_pendingHeight > 0)
                emit videoSizeChanged(m_pendingWidth, m_pendingHeight);
        }
    }
}

} // namespace colorfy
