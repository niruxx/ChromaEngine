#include "colorfy/ConfigStore.h"

#include <QDir>
#include <QFile>
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

    QFile file(configFilePath());
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

} // namespace colorfy
