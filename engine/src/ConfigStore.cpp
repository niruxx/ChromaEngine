#include "colorfy/ConfigStore.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace colorfy {

QString ConfigStore::configFilePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/config.json");
}

MediaItem ConfigStore::load()
{
    MediaItem item;
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return item;

    const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    item.filePath = obj.value(QStringLiteral("filePath")).toString();
    item.libraryFolder = obj.value(QStringLiteral("libraryFolder")).toString();
    item.muted = obj.value(QStringLiteral("muted")).toBool(true);
    item.volume = obj.value(QStringLiteral("volume")).toInt(100);
    item.fitMode = static_cast<FitMode>(obj.value(QStringLiteral("fitMode")).toInt(0));

    item.flipHorizontal = obj.value(QStringLiteral("flipHorizontal")).toBool(false);
    item.flipVertical = obj.value(QStringLiteral("flipVertical")).toBool(false);
    item.brightness = obj.value(QStringLiteral("brightness")).toInt(0);
    item.contrast = obj.value(QStringLiteral("contrast")).toInt(0);
    item.saturation = obj.value(QStringLiteral("saturation")).toInt(0);
    item.playbackRate = obj.value(QStringLiteral("playbackRate")).toDouble(1.0);
    item.zoom = obj.value(QStringLiteral("zoom")).toDouble(0.0);

    item.launchAtStartup = obj.value(QStringLiteral("launchAtStartup")).toBool(false);
    item.closeToTray = obj.value(QStringLiteral("closeToTray")).toBool(true);
    item.startMinimized = obj.value(QStringLiteral("startMinimized")).toBool(false);
    item.backgroundTheme = obj.value(QStringLiteral("backgroundTheme")).toInt(0);
    item.showDesktopIcons = obj.value(QStringLiteral("showDesktopIcons")).toBool(true);

    item.memoryLimitEnabled = obj.value(QStringLiteral("memoryLimitEnabled")).toBool(false);
    item.memoryLimitMb = obj.value(QStringLiteral("memoryLimitMb")).toInt(4096);
    item.softwareRendering = obj.value(QStringLiteral("softwareRendering")).toBool(false);
    item.previewFrameRateLimit = obj.value(QStringLiteral("previewFrameRateLimit")).toInt(0);
    item.thumbnailAutoPlayEnabled = obj.value(QStringLiteral("thumbnailAutoPlayEnabled")).toBool(false);

    item.clockEnabled = obj.value(QStringLiteral("clockEnabled")).toBool(false);
    item.clockLayout = static_cast<ClockLayout>(obj.value(QStringLiteral("clockLayout")).toInt(0));
    item.clockTheme = static_cast<OverlayTheme>(obj.value(QStringLiteral("clockTheme")).toInt(0));
    item.clockFontFamily = obj.value(QStringLiteral("clockFontFamily")).toString();
    item.clockFontSize = obj.value(QStringLiteral("clockFontSize")).toInt(48);
    item.clockRotation = obj.value(QStringLiteral("clockRotation")).toInt(0);
    item.clockPosition = static_cast<OverlayPosition>(obj.value(QStringLiteral("clockPosition")).toInt(1));
    item.clockMargin = obj.value(QStringLiteral("clockMargin")).toInt(40);

    item.calendarEnabled = obj.value(QStringLiteral("calendarEnabled")).toBool(false);
    item.calendarTheme = static_cast<OverlayTheme>(obj.value(QStringLiteral("calendarTheme")).toInt(0));
    item.calendarPosition = static_cast<OverlayPosition>(obj.value(QStringLiteral("calendarPosition")).toInt(3));
    item.calendarMargin = obj.value(QStringLiteral("calendarMargin")).toInt(40);

    item.batteryIndicatorEnabled = obj.value(QStringLiteral("batteryIndicatorEnabled")).toBool(false);
    item.batteryTheme = static_cast<OverlayTheme>(obj.value(QStringLiteral("batteryTheme")).toInt(0));
    item.batteryPosition = static_cast<OverlayPosition>(obj.value(QStringLiteral("batteryPosition")).toInt(0));
    item.batteryMargin = obj.value(QStringLiteral("batteryMargin")).toInt(40);

    for (const QJsonValue& value : obj.value(QStringLiteral("enabledMonitorIds")).toArray())
        item.enabledMonitorIds.append(value.toString());

    return item;
}

void ConfigStore::save(const MediaItem& item)
{
    QJsonObject obj;
    obj[QStringLiteral("filePath")] = item.filePath;
    obj[QStringLiteral("libraryFolder")] = item.libraryFolder;
    obj[QStringLiteral("muted")] = item.muted;
    obj[QStringLiteral("volume")] = item.volume;
    obj[QStringLiteral("fitMode")] = static_cast<int>(item.fitMode);

    obj[QStringLiteral("flipHorizontal")] = item.flipHorizontal;
    obj[QStringLiteral("flipVertical")] = item.flipVertical;
    obj[QStringLiteral("brightness")] = item.brightness;
    obj[QStringLiteral("contrast")] = item.contrast;
    obj[QStringLiteral("saturation")] = item.saturation;
    obj[QStringLiteral("playbackRate")] = item.playbackRate;
    obj[QStringLiteral("zoom")] = item.zoom;

    obj[QStringLiteral("launchAtStartup")] = item.launchAtStartup;
    obj[QStringLiteral("closeToTray")] = item.closeToTray;
    obj[QStringLiteral("startMinimized")] = item.startMinimized;
    obj[QStringLiteral("backgroundTheme")] = item.backgroundTheme;
    obj[QStringLiteral("showDesktopIcons")] = item.showDesktopIcons;

    obj[QStringLiteral("memoryLimitEnabled")] = item.memoryLimitEnabled;
    obj[QStringLiteral("memoryLimitMb")] = item.memoryLimitMb;
    obj[QStringLiteral("softwareRendering")] = item.softwareRendering;
    obj[QStringLiteral("previewFrameRateLimit")] = item.previewFrameRateLimit;
    obj[QStringLiteral("thumbnailAutoPlayEnabled")] = item.thumbnailAutoPlayEnabled;

    obj[QStringLiteral("clockEnabled")] = item.clockEnabled;
    obj[QStringLiteral("clockLayout")] = static_cast<int>(item.clockLayout);
    obj[QStringLiteral("clockTheme")] = static_cast<int>(item.clockTheme);
    obj[QStringLiteral("clockFontFamily")] = item.clockFontFamily;
    obj[QStringLiteral("clockFontSize")] = item.clockFontSize;
    obj[QStringLiteral("clockRotation")] = item.clockRotation;
    obj[QStringLiteral("clockPosition")] = static_cast<int>(item.clockPosition);
    obj[QStringLiteral("clockMargin")] = item.clockMargin;

    obj[QStringLiteral("calendarEnabled")] = item.calendarEnabled;
    obj[QStringLiteral("calendarTheme")] = static_cast<int>(item.calendarTheme);
    obj[QStringLiteral("calendarPosition")] = static_cast<int>(item.calendarPosition);
    obj[QStringLiteral("calendarMargin")] = item.calendarMargin;

    obj[QStringLiteral("batteryIndicatorEnabled")] = item.batteryIndicatorEnabled;
    obj[QStringLiteral("batteryTheme")] = static_cast<int>(item.batteryTheme);
    obj[QStringLiteral("batteryPosition")] = static_cast<int>(item.batteryPosition);
    obj[QStringLiteral("batteryMargin")] = item.batteryMargin;

    obj[QStringLiteral("enabledMonitorIds")] = QJsonArray::fromStringList(item.enabledMonitorIds);

    QFile file(configFilePath());
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

} // namespace colorfy
