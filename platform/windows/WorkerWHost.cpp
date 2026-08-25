#include "WorkerWHost.h"

#include <QDebug>
#include <QTimer>

#include <windows.h>

namespace colorfy {

namespace {

BOOL CALLBACK findWorkerWProc(HWND hwnd, LPARAM lparam)
{
    HWND shellView = FindWindowExW(hwnd, nullptr, L"SHELLDLL_DefView", nullptr);
    if (shellView) {
        // The WorkerW that should host our wallpaper is the next WorkerW
        // sibling after the one holding the desktop icons.
        HWND* result = reinterpret_cast<HWND*>(lparam);
        *result = FindWindowExW(nullptr, hwnd, L"WorkerW", nullptr);
        return FALSE;
    }
    return TRUE;
}

} // namespace

WorkerWHost::WorkerWHost(QObject* parent)
    : QObject(parent)
{
}

void* WorkerWHost::locateWorkerW()
{
    HWND progman = FindWindowW(L"Progman", nullptr);
    if (!progman) {
        qWarning() << "Progman window not found";
        return nullptr;
    }

    // Undocumented message that makes Progman spawn the WorkerW behind the
    // desktop icons if it hasn't already. Used by every wallpaper-engine-like
    // app on Windows; there is no public API for this.
    SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);

    HWND workerW = nullptr;
    EnumWindows(findWorkerWProc, reinterpret_cast<LPARAM>(&workerW));

    if (!workerW) {
        // Some Windows builds parent the desktop icons directly under
        // Progman without a separate WorkerW sibling; fall back to it.
        workerW = progman;
    }

    return workerW;
}

void* WorkerWHost::attach()
{
    m_workerW = locateWorkerW();
    if (m_workerW)
        emit workerWChanged(m_workerW);
    else
        qWarning() << "Failed to locate WorkerW";
    return m_workerW;
}

void WorkerWHost::startWatchdog(int intervalMs)
{
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &WorkerWHost::checkHealth);
    timer->start(intervalMs);
}

void WorkerWHost::checkHealth()
{
    HWND current = reinterpret_cast<HWND>(m_workerW);
    if (current && IsWindow(current))
        return;

    qWarning() << "WorkerW lost (explorer.exe restarted?), re-attaching";
    attach();
}

} // namespace colorfy
