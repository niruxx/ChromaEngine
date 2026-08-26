#include "BluetoothBatteryReader.h"

#include <initguid.h>
#include <windows.h>

#include <devpkey.h>
#include <setupapi.h>

#pragma comment(lib, "setupapi.lib")

namespace colorfy {

namespace {

// Not reliably declared across Windows SDK versions, so defined locally
// under our own name rather than risking a redefinition clash with whatever
// devpkey.h/bthdef.h may or may not provide it as. Value is the documented,
// stable DEVPKEY_Bluetooth_Battery GUID/PID.
DEFINE_DEVPROPKEY(kDevPKeyBluetoothBattery, 0x104ea319, 0x6ee2, 0x4701, 0xbd, 0x47, 0x8d, 0xdb, 0xf4, 0x25, 0xbb, 0xe5,
                   2);

} // namespace

QList<BluetoothDeviceBattery> BluetoothBatteryReader::queryConnectedDevices()
{
    QList<BluetoothDeviceBattery> results;

    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return results;

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD index = 0; SetupDiEnumDeviceInfo(deviceInfoSet, index, &deviceInfoData); ++index) {
        DEVPROPTYPE propertyType = 0;
        BYTE batteryValue = 0;
        DWORD requiredSize = 0;

        const BOOL hasBattery =
            SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &kDevPKeyBluetoothBattery, &propertyType,
                                       &batteryValue, sizeof(batteryValue), &requiredSize, 0);
        if (!hasBattery || propertyType != DEVPROP_TYPE_BYTE)
            continue;

        wchar_t nameBuffer[256] = {};
        DWORD nameSize = 0;
        SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_FriendlyName, &propertyType,
                                   reinterpret_cast<BYTE*>(nameBuffer), sizeof(nameBuffer) - sizeof(wchar_t),
                                   &nameSize, 0);

        BluetoothDeviceBattery device;
        device.name = nameSize > 0 ? QString::fromWCharArray(nameBuffer) : QStringLiteral("Bluetooth device");
        device.batteryPercent = static_cast<int>(batteryValue);
        results.append(device);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return results;
}

} // namespace colorfy
