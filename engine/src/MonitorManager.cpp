#include "colorfy/MonitorManager.h"

#include <QGuiApplication>
#include <QScreen>

#ifdef _WIN32
#include <windows.h>
#endif

namespace colorfy {

QRect MonitorManager::primaryGeometry()
{
#ifdef _WIN32
    // Query Windows directly for the true physical pixel size. Going through
    // QScreen::geometry() here would return Qt's own high-DPI-scaled logical
    // pixels, which don't match what the raw Win32 SetWindowPos call in
    // WallpaperWindow needs to actually cover the full physical screen.
    const int width = GetSystemMetrics(SM_CXSCREEN);
    const int height = GetSystemMetrics(SM_CYSCREEN);
    if (width > 0 && height > 0)
        return QRect(0, 0, width, height);
#endif

    if (QScreen* screen = QGuiApplication::primaryScreen())
        return screen->geometry();
    return QRect(0, 0, 1920, 1080);
}

QList<QRect> MonitorManager::allGeometries()
{
    QList<QRect> geometries;
    for (QScreen* screen : QGuiApplication::screens())
        geometries.append(screen->geometry());
    return geometries;
}

#ifdef _WIN32
namespace {

BOOL CALLBACK enumMonitorProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam)
{
    auto* result = reinterpret_cast<QList<MonitorInfo>*>(lParam);

    MONITORINFOEXW info;
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(hMonitor, &info))
        return TRUE;

    MonitorInfo monitor;
    monitor.id = QString::fromWCharArray(info.szDevice);
    monitor.isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
    monitor.geometry = QRect(info.rcMonitor.left, info.rcMonitor.top,
                              info.rcMonitor.right - info.rcMonitor.left,
                              info.rcMonitor.bottom - info.rcMonitor.top);
    monitor.name = QStringLiteral("%1 (%2x%3)%4")
                       .arg(monitor.id)
                       .arg(monitor.geometry.width())
                       .arg(monitor.geometry.height())
                       .arg(monitor.isPrimary ? QStringLiteral(" — Primary") : QString());

    result->append(monitor);
    return TRUE;
}

} // namespace
#endif

QList<MonitorInfo> MonitorManager::listMonitors()
{
    QList<MonitorInfo> monitors;

#ifdef _WIN32
    EnumDisplayMonitors(nullptr, nullptr, enumMonitorProc, reinterpret_cast<LPARAM>(&monitors));
#else
    // No native per-monitor enumeration API to go through here the way
    // Windows' EnumDisplayMonitors needs (X11/XRandR's own info is exactly
    // what QScreen is already built on) - QGuiApplication::screens() gives
    // real per-monitor geometry directly.
    QScreen* primary = QGuiApplication::primaryScreen();
    for (QScreen* screen : QGuiApplication::screens()) {
        MonitorInfo monitor;
        // QScreen::name() is the output name (e.g. "eDP-1", "HDMI-1") on
        // X11/XRandR - stable across sessions, unlike a positional index.
        monitor.id = screen->name();
        monitor.isPrimary = (screen == primary);
        monitor.geometry = screen->geometry();
        monitor.name = QStringLiteral("%1 (%2x%3)%4")
                           .arg(monitor.id)
                           .arg(monitor.geometry.width())
                           .arg(monitor.geometry.height())
                           .arg(monitor.isPrimary ? QStringLiteral(" — Primary") : QString());
        monitors.append(monitor);
    }
#endif

    if (monitors.isEmpty()) {
        MonitorInfo fallback;
        fallback.id = QStringLiteral("primary");
        fallback.geometry = primaryGeometry();
        fallback.isPrimary = true;
        fallback.name = QStringLiteral("Primary Display");
        monitors.append(fallback);
    }

    return monitors;
}

} // namespace colorfy
