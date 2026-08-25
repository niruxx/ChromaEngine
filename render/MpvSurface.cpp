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
    mpv_set_option_string(m_mpv, "keepaspect", "yes");
    mpv_set_option_string(m_mpv, "panscan", "1.0");
    mpv_set_option_string(m_mpv, "osc", "no");
    mpv_set_option_string(m_mpv, "osd-level", "0");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    mpv_set_option_string(m_mpv, "cursor-autohide", "no");

    checkError(mpv_initialize(m_mpv), "mpv_initialize");

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
    mpv_set_option_string(m_mpv, "mute", muted ? "yes" : "no");
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

    switch (mode) {
    case FitMode::Fill:
        mpv_set_option_string(m_mpv, "keepaspect", "yes");
        mpv_set_option_string(m_mpv, "panscan", "1.0");
        mpv_set_option_string(m_mpv, "video-unscaled", "no");
        break;
    case FitMode::Fit:
        mpv_set_option_string(m_mpv, "keepaspect", "yes");
        mpv_set_option_string(m_mpv, "panscan", "0.0");
        mpv_set_option_string(m_mpv, "video-unscaled", "no");
        break;
    case FitMode::Stretch:
        mpv_set_option_string(m_mpv, "keepaspect", "no");
        mpv_set_option_string(m_mpv, "video-unscaled", "no");
        break;
    case FitMode::Center:
        mpv_set_option_string(m_mpv, "keepaspect", "yes");
        mpv_set_option_string(m_mpv, "panscan", "0.0");
        mpv_set_option_string(m_mpv, "video-unscaled", "yes");
        break;
    }
}

void MpvSurface::pumpEvents()
{
    if (!m_mpv)
        return;

    while (true) {
        mpv_event* event = mpv_wait_event(m_mpv, 0);
        if (!event || event->event_id == MPV_EVENT_NONE)
            break;
    }
}

} // namespace colorfy
