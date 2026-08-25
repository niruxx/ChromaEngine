#include "WallpaperWindow.h"

#include <windows.h>

namespace colorfy {

WallpaperWindow::WallpaperWindow(const QRect& geometry, QWidget* parent)
    : QWidget(parent)
    , m_geometry(geometry)
{
    setAttribute(Qt::WA_NativeWindow);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAutoFillBackground(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);

    setGeometry(m_geometry);

    // Force native HWND creation now so winId() is valid before reparenting.
    winId();
}

void* WallpaperWindow::nativeHandle() const
{
    return reinterpret_cast<void*>(winId());
}

void WallpaperWindow::attachToWorkerW(void* workerWHandle)
{
    HWND hwnd = reinterpret_cast<HWND>(winId());
    HWND workerW = reinterpret_cast<HWND>(workerWHandle);
    if (!hwnd || !workerW)
        return;

    SetParent(hwnd, workerW);

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
    style |= WS_CHILD;
    SetWindowLongPtrW(hwnd, GWL_STYLE, style);

    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_APPWINDOW | WS_EX_TOOLWINDOW);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);

    SetWindowPos(hwnd, nullptr,
                 m_geometry.x(), m_geometry.y(), m_geometry.width(), m_geometry.height(),
                 SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
}

} // namespace colorfy
