#pragma once

#include <QList>
#include <QRect>
#include <QString>

namespace colorfy {

struct MonitorInfo {
    QString id; // stable device name, e.g. "\\.\DISPLAY1"
    QString name; // human-readable label for UI
    QRect geometry; // true physical pixels
    bool isPrimary = false;
};

class MonitorManager {
public:
    static QRect primaryGeometry();
    static QList<QRect> allGeometries();
    static QList<MonitorInfo> listMonitors();
};

} // namespace colorfy
