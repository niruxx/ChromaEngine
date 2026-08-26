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

    // m_geometry is in absolute virtual-screen coordinates (from
    // MonitorManager's GetMonitorInfo query), but SetWindowPos on a
    // WS_CHILD window places it relative to the parent's own client-area
    // origin - not the same thing whenever the virtual desktop's origin
    // isn't (0,0), which happens as soon as any monitor sits above/left of
    // the primary. Without this adjustment the window lands offset by
    // exactly that difference (confirmed live: a monitor above the primary
    // shifted the wallpaper up, leaving a gap of exposed real desktop at
    // the bottom and pushing part of the video off the top).
    RECT parentRect{};
    GetWindowRect(workerW, &parentRect);

    SetWindowPos(hwnd, nullptr,
                 m_geometry.x() - parentRect.left, m_geometry.y() - parentRect.top,
                 m_geometry.width(), m_geometry.height(),
                 SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    reorderRelativeToIcons();
}

void WallpaperWindow::setShowDesktopIcons(bool show)
{
    m_showDesktopIcons = show;
    reorderRelativeToIcons();
}

void WallpaperWindow::reorderRelativeToIcons()
{
    HWND hwnd = reinterpret_cast<HWND>(winId());
    HWND parent = GetParent(hwnd);
    if (!hwnd || !parent)
        return;

    // Only relevant if the icon view actually lives under the same parent as
    // us (some Windows configurations put SHELLDLL_DefView directly under
    // Progman with no separate WorkerW, making it our sibling rather than
    // living in an entirely separate window WorkerWHost can stack against).
    HWND shellView = FindWindowExW(parent, nullptr, L"SHELLDLL_DefView", nullptr);
    if (!shellView)
        return;

    if (m_showDesktopIcons)
        SetWindowPos(hwnd, shellView, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    else
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

} // namespace colorfy
