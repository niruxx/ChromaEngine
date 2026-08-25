#pragma once

#include <QObject>

class QSystemTrayIcon;
class QMenu;
class QAction;

namespace colorfy {

class TrayIcon : public QObject {
    Q_OBJECT
public:
    explicit TrayIcon(QObject* parent = nullptr);

signals:
    void openSettingsRequested();
    void pauseToggled(bool paused);
    void quitRequested();

private:
    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_menu = nullptr;
    QAction* m_pauseAction = nullptr;
};

} // namespace colorfy
