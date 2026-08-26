#pragma once

#include <QList>
#include <QString>

namespace colorfy {

struct BluetoothDeviceBattery {
    QString name;
    int batteryPercent = -1; // 0..100, or -1 if the device didn't report one
};

// Reads battery levels Windows already knows about for paired Bluetooth
// devices (the same data shown in Settings > Bluetooth & devices), via the
// DEVPKEY_Bluetooth_Battery device property - no separate BLE GATT
// connection or pairing logic needed, since Windows already maintains this
// for any device that reports it.
class BluetoothBatteryReader {
public:
    static QList<BluetoothDeviceBattery> queryConnectedDevices();
};

} // namespace colorfy
