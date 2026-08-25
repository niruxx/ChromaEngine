#include "colorfy/LibraryScanner.h"

#include <QDir>
#include <QFileInfo>

namespace colorfy {

QStringList LibraryScanner::scan(const QString& folder)
{
    if (folder.isEmpty())
        return {};

    static const QStringList filters = {
        QStringLiteral("*.mp4"), QStringLiteral("*.mkv"), QStringLiteral("*.webm"),
        QStringLiteral("*.avi"), QStringLiteral("*.mov"), QStringLiteral("*.gif"),
    };

    QDir dir(folder);
    const QFileInfoList entries = dir.entryInfoList(filters, QDir::Files, QDir::Name | QDir::IgnoreCase);

    QStringList paths;
    paths.reserve(entries.size());
    for (const QFileInfo& info : entries)
        paths.append(info.absoluteFilePath());
    return paths;
}

bool LibraryScanner::isGif(const QString& filePath)
{
    return filePath.endsWith(QStringLiteral(".gif"), Qt::CaseInsensitive);
}

} // namespace colorfy
