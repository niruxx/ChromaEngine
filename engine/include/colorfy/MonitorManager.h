#pragma once

#include <QList>
#include <QRect>

namespace colorfy {

class MonitorManager {
public:
    static QRect primaryGeometry();
    static QList<QRect> allGeometries();
};

} // namespace colorfy
