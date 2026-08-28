#include "BluetoothBatteryReader.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMap>
#include <QVariantMap>

namespace colorfy {

namespace {

// One BlueZ object (an interface-name -> properties map) as returned by
// org.freedesktop.DBus.ObjectManager.GetManagedObjects, keyed by D-Bus
// object path. Matches the standard "a{oa{sa{sv}}}" ObjectManager signature
// - QtDBus's generic QMap marshalling handles the nested a{sv} -> QVariantMap
// conversion for us.
using InterfaceProperties = QMap<QString, QVariantMap>;
using ManagedObjects = QMap<QDBusObjectPath, InterfaceProperties>;

} // namespace

QList<BluetoothDeviceBattery> BluetoothBatteryReader::queryConnectedDevices()
{
    QList<BluetoothDeviceBattery> results;

    QDBusMessage call = QDBusMessage::createMethodCall(
        QStringLiteral("org.bluez"), QStringLiteral("/"), QStringLiteral("org.freedesktop.DBus.ObjectManager"),
        QStringLiteral("GetManagedObjects"));
    const QDBusMessage reply = QDBusConnection::systemBus().call(call);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty())
        return results; // bluetoothd not running / no system bus access - degrade to "no devices"

    ManagedObjects objects;
    reply.arguments().constFirst().value<QDBusArgument>() >> objects;

    for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
        const InterfaceProperties& interfaces = it.value();
        if (!interfaces.contains(QStringLiteral("org.bluez.Battery1")))
            continue;

        BluetoothDeviceBattery device;
        device.batteryPercent = interfaces.value(QStringLiteral("org.bluez.Battery1"))
                                     .value(QStringLiteral("Percentage"))
                                     .toInt();

        const QVariantMap deviceProps = interfaces.value(QStringLiteral("org.bluez.Device1"));
        device.name = deviceProps.value(QStringLiteral("Alias")).toString();
        if (device.name.isEmpty())
            device.name = deviceProps.value(QStringLiteral("Name")).toString();
        if (device.name.isEmpty())
            device.name = QStringLiteral("Bluetooth device");

        results.append(device);
    }

    return results;
}

} // namespace colorfy
