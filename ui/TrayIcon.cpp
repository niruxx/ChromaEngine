#include "TrayIcon.h"

#include "IconFactory.h"

#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTextStream>

namespace colorfy {

namespace {

// TEMPORARY diagnostic: pinpointing a reported freeze (spinning cursor,
// rest of the OS stays responsive) when closing to tray then right-
// clicking the tray icon. Shares one log file with SettingsWindow.cpp and
// main_windows.cpp's heartbeat so the whole sequence interleaves into one
// timeline. Remove once resolved.
void chromaDebugLog(const QString& line)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(dir);
    QFile f(dir + QStringLiteral("/chroma_debug.log"));
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " " << line << Qt::endl;
    }
}

} // namespace

TrayIcon::TrayIcon(QObject* parent)
    : QObject(parent)
{
    m_menu = new QMenu();

    auto* settingsAction = m_menu->addAction(QStringLiteral("Settings..."));
    connect(settingsAction, &QAction::triggered, this, &TrayIcon::openSettingsRequested);

    m_pauseAction = m_menu->addAction(QStringLiteral("Pause"));
    m_pauseAction->setCheckable(true);
    connect(m_pauseAction, &QAction::toggled, this, &TrayIcon::pauseToggled);

    m_menu->addSeparator();

    auto* quitAction = m_menu->addAction(QStringLiteral("Quit"));
    connect(quitAction, &QAction::triggered, this, &TrayIcon::quitRequested);

    connect(m_menu, &QMenu::aboutToShow, this, [] { chromaDebugLog(QStringLiteral("TrayIcon: menu aboutToShow")); });
    connect(m_menu, &QMenu::aboutToHide, this, [] { chromaDebugLog(QStringLiteral("TrayIcon: menu aboutToHide")); });

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(IconFactory::appLogo(32));
    m_trayIcon->setToolTip(QStringLiteral("ChromaEngine"));
    m_trayIcon->setContextMenu(m_menu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        chromaDebugLog(QStringLiteral("TrayIcon: activated reason=%1").arg(static_cast<int>(reason)));
        if (reason == QSystemTrayIcon::DoubleClick)
            emit openSettingsRequested();
    });
}

} // namespace colorfy
