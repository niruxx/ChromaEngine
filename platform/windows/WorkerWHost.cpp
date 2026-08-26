#include "WorkerWHost.h"

#include <QDebug>
#include <QTimer>

#include <windows.h>

namespace colorfy {

namespace {

struct FindResult {
    HWND iconHostWorkerW = nullptr; // hosts SHELLDLL_DefView (the icons)
    HWND targetWorkerW = nullptr; // the empty one, behind the icons
};

BOOL CALLBACK findWorkerWProc(HWND hwnd, LPARAM lparam)
{
    auto* result = reinterpret_cast<FindResult*>(lparam);

    HWND shellView = FindWindowExW(hwnd, nullptr, L"SHELLDLL_DefView", nullptr);
    if (shellView) {
        result->iconHostWorkerW = hwnd;
        // On most Windows builds the empty WorkerW meant for wallpaper
        // content is enumerated immediately after this one, but that
        // ordering isn't documented or guaranteed - enforceStacking() below
        // corrects the actual on-screen stacking regardless of whether this
        // guess is right.
        result->targetWorkerW = FindWindowExW(nullptr, hwnd, L"WorkerW", nullptr);
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

    FindResult found;
    EnumWindows(findWorkerWProc, reinterpret_cast<LPARAM>(&found));

    m_iconHostWorkerW = found.iconHostWorkerW;

    HWND workerW = found.targetWorkerW;
    if (!workerW) {
        // Some Windows builds parent the desktop icons directly under
        // Progman without a separate WorkerW sibling; fall back to it.
        workerW = progman;
    }

    return workerW;
}

void WorkerWHost::setShowDesktopIcons(bool show)
{
    m_showDesktopIcons = show;
    enforceStacking();
}

void WorkerWHost::enforceStacking()
{
    // Windows doesn't guarantee which of the two WorkerW windows ends up on
    // top - on some builds/updates the empty one we render into ends up in
    // FRONT of the one holding the desktop icons, which visibly covers them.
    // Forcing our target's z-order relative to the icon host fixes that
    // regardless of whatever order they were actually created in, and lets
    // the "hide icons under the wallpaper" option work the same way in
    // reverse. Only our own window's z-order is ever touched here, never
    // Explorer's.
    HWND target = reinterpret_cast<HWND>(m_workerW);
    HWND iconHost = reinterpret_cast<HWND>(m_iconHostWorkerW);
    if (!target || target == iconHost)
        return;

    if (m_showDesktopIcons && iconHost) {
        SetWindowPos(target, iconHost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    } else {
        SetWindowPos(target, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void* WorkerWHost::attach()
{
    m_workerW = locateWorkerW();
    if (m_workerW) {
        enforceStacking();
        emit workerWChanged(m_workerW);
    } else {
        qWarning() << "Failed to locate WorkerW";
    }
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
    if (current && IsWindow(current)) {
        // Cheap to re-assert every tick, and guards against Explorer
        // shuffling WorkerW z-order during normal use (e.g. "Show Desktop").
        enforceStacking();
        return;
    }

    qWarning() << "WorkerW lost (explorer.exe restarted?), re-attaching";
    attach();
}

} // namespace colorfy
