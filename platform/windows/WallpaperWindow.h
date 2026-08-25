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

private:
    QRect m_geometry;
};

} // namespace colorfy
