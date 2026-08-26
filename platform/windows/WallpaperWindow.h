#pragma once

#include <QRect>
#include <QWidget>

namespace colorfy {

// A borderless native window sized to a monitor, meant to be reparented into
// the WorkerW window that lives behind the desktop icons.
class WallpaperWindow : public QWidget {
    Q_OBJECT
public:
    explicit WallpaperWindow(const QRect& geometry, QWidget* parent = nullptr);

    void* nativeHandle() const;
    void attachToWorkerW(void* workerWHandle);

    // Some Windows configurations put the desktop icons (SHELLDLL_DefView)
    // directly under Progman with no separate WorkerW at all, meaning our
    // window becomes a *sibling* of the icon view rather than living in a
    // wholly separate window - WorkerWHost's cross-window stacking fix can't
    // help there, since there's only one window to stack. This handles that
    // case by reordering directly against a SHELLDLL_DefView sibling under
    // our own parent, if one exists (a no-op otherwise).
    void setShowDesktopIcons(bool show);

private:
    void reorderRelativeToIcons();

    QRect m_geometry;
    bool m_showDesktopIcons = true;
};

} // namespace colorfy
