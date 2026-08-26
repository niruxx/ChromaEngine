#pragma once

#include <QDialog>
#include <QList>
#include <QStringList>

#include "colorfy/MonitorManager.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QSlider;
class QSpinBox;
class QFontComboBox;
class QVBoxLayout;

namespace colorfy {

// Preferences dialog: General (startup, close behavior, default wallpaper
// folder), Displays (which monitors the wallpaper is applied to),
// Appearance (library background theme), and Personalization (clock/
// calendar/Bluetooth-battery desktop overlay widgets). Every toggle applies
// immediately - there's no separate OK/Cancel/Apply step, matching the rest
// of the app.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void setLaunchAtStartup(bool enabled);
    void setCloseToTray(bool enabled);
    void setStartMinimized(bool enabled);
    void setThumbnailAutoPlay(bool enabled);
    void setDefaultFolder(const QString& folder);
    void setBackgroundTheme(int themeIndex);
    void setMonitors(const QList<MonitorInfo>& monitors, const QStringList& enabledIds);
    void setMemoryLimit(bool enabled, int limitMb);

    void setClockSettings(bool enabled, int layout, int theme, const QString& fontFamily, int fontSize, int rotation,
                           int position, int margin);
    void setCalendarSettings(bool enabled, int theme, int position, int margin);
    void setBatterySettings(bool enabled, int theme, int position, int margin);

signals:
    void launchAtStartupToggled(bool enabled);
    void closeToTrayToggled(bool enabled);
    void startMinimizedToggled(bool enabled);
    void thumbnailAutoPlayToggled(bool enabled);
    void defaultFolderChanged(const QString& folder);
    void backgroundThemeChanged(int themeIndex);
    void monitorToggled(const QString& monitorId, bool enabled);
    void memoryLimitChanged(bool enabled, int limitMb);

    void clockSettingsChanged(bool enabled, int layout, int theme, const QString& fontFamily, int fontSize,
                               int rotation, int position, int margin);
    void calendarSettingsChanged(bool enabled, int theme, int position, int margin);
    void batterySettingsChanged(bool enabled, int theme, int position, int margin);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void buildUi();
    QWidget* buildPersonalizationTab();
    void emitClockSettings();
    void emitCalendarSettings();
    void emitBatterySettings();

    QCheckBox* m_launchAtStartupCheck = nullptr;
    QCheckBox* m_closeToTrayCheck = nullptr;
    QCheckBox* m_startMinimizedCheck = nullptr;
    QCheckBox* m_thumbnailAutoPlayCheck = nullptr;
    QLabel* m_defaultFolderLabel = nullptr;
    QComboBox* m_backgroundThemeCombo = nullptr;
    QVBoxLayout* m_displaysLayout = nullptr;

    QCheckBox* m_memoryLimitCheck = nullptr;
    QSpinBox* m_memoryLimitSpin = nullptr;
    QLabel* m_memoryLimitNote = nullptr;

    QCheckBox* m_clockEnabledCheck = nullptr;
    QComboBox* m_clockLayoutCombo = nullptr;
    QComboBox* m_clockThemeCombo = nullptr;
    QFontComboBox* m_clockFontCombo = nullptr;
    QSlider* m_clockFontSizeSlider = nullptr;
    QComboBox* m_clockRotationCombo = nullptr;
    QComboBox* m_clockPositionCombo = nullptr;
    QSlider* m_clockMarginSlider = nullptr;

    QCheckBox* m_calendarEnabledCheck = nullptr;
    QComboBox* m_calendarThemeCombo = nullptr;
    QComboBox* m_calendarPositionCombo = nullptr;
    QSlider* m_calendarMarginSlider = nullptr;

    QCheckBox* m_batteryEnabledCheck = nullptr;
    QComboBox* m_batteryThemeCombo = nullptr;
    QComboBox* m_batteryPositionCombo = nullptr;
    QSlider* m_batteryMarginSlider = nullptr;

    QString m_defaultFolder;
};

} // namespace colorfy
