#include "StartupManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

namespace colorfy {

namespace {

QString registryPath()
{
    return QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
}

QString valueName()
{
    return QStringLiteral("ChromaEngine");
}

} // namespace

bool StartupManager::isEnabled()
{
    QSettings settings(registryPath(), QSettings::NativeFormat);
    return settings.contains(valueName());
}

void StartupManager::setEnabled(bool enabled)
{
    QSettings settings(registryPath(), QSettings::NativeFormat);
    if (enabled) {
        const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        settings.setValue(valueName(), QStringLiteral("\"%1\"").arg(exePath));
    } else {
        settings.remove(valueName());
    }
}

} // namespace colorfy
