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

} // namespace colorfy
