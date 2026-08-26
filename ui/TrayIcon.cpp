#include "TrayIcon.h"

#include "IconFactory.h"

#include <QAction>
#include <QMenu>
#include <QSystemTrayIcon>

namespace colorfy {

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

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(IconFactory::appLogo(32));
    m_trayIcon->setToolTip(QStringLiteral("ChromaEngine"));
    m_trayIcon->setContextMenu(m_menu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick)
            emit openSettingsRequested();
    });
}

} // namespace colorfy
