#pragma once

#include <QList>
#include <QString>

namespace colorfy {

struct BluetoothDeviceBattery {
    QString name;
    int batteryPercent = -1; // 0..100, or -1 if the device didn't report one
};

// Reads battery levels for paired Bluetooth devices via BlueZ's D-Bus API
// (org.bluez.Battery1, exposed per-device by the same daemon that already
// tracks pairing/connection state) - no separate BLE GATT connection or
// pairing logic needed, mirroring how the Windows build reads the battery
// value Windows itself already maintains rather than querying the device
// directly.
class BluetoothBatteryReader {
public:
    static QList<BluetoothDeviceBattery> queryConnectedDevices();
};

} // namespace colorfy
