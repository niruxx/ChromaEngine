#include "StartupManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

namespace colorfy {

namespace {

QString autostartFilePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/autostart");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/chroma-engine.desktop");
}

} // namespace

bool StartupManager::isEnabled()
{
    return QFile::exists(autostartFilePath());
}

void StartupManager::setEnabled(bool enabled)
{
    const QString path = autostartFilePath();

    if (!enabled) {
        QFile::remove(path);
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "[Desktop Entry]\n"
        << "Type=Application\n"
        << "Name=ChromaEngine\n"
        << "Comment=Animated video/GIF wallpaper engine\n"
        << "Exec=\"" << QCoreApplication::applicationFilePath() << "\"\n"
        << "Icon=chroma-engine\n"
        << "Terminal=false\n"
        << "X-GNOME-Autostart-enabled=true\n";
}

} // namespace colorfy
