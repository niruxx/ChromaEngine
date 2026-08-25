#pragma once

#include <QObject>

namespace colorfy {

// Finds (and, if necessary, forces Windows to spawn) the hidden WorkerW
// window that lives behind the desktop icons, so a wallpaper window can be
// reparented into it. Watches for explorer.exe restarts, which destroy and
// recreate WorkerW, and re-attaches automatically.
class WorkerWHost : public QObject {
    Q_OBJECT
public:
    explicit WorkerWHost(QObject* parent = nullptr);

    // Locates (or re-locates) the WorkerW handle. Returns nullptr on failure.
    void* attach();

    void startWatchdog(int intervalMs = 2000);

signals:
    void workerWChanged(void* hwnd);

private slots:
    void checkHealth();

private:
    void* locateWorkerW();

    void* m_workerW = nullptr;
};

} // namespace colorfy
