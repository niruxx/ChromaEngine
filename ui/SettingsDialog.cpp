#include "SettingsDialog.h"

#include "AnimationHelpers.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFontComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace colorfy {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Preferences"));
    setStyleSheet(QStringLiteral(R"(
        QDialog { background-color: #17171a; }
        QWidget { color: #e6e6e6; }
        QTabWidget::pane { border: 1px solid #2a2a2e; border-radius: 6px; top: -1px; }
        QTabBar::tab {
            background: #1c1c20;
            padding: 8px 16px;
            border: 1px solid #2a2a2e;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
        QTabBar::tab:selected { background: #24242a; }
        QCheckBox::indicator {
            width: 16px; height: 16px;
            border-radius: 4px;
            border: 1px solid #4a4a52;
            background-color: #222226;
        }
        QCheckBox::indicator:checked { background-color: #5ba8e6; border-color: #5ba8e6; }
        QPushButton {
            background-color: #24242a;
            border: 1px solid #34343c;
            border-radius: 6px;
            padding: 5px 10px;
        }
        QPushButton:hover { background-color: #2b2b32; }
        QComboBox {
            background-color: #222226;
            border: 1px solid #3a3a40;
            border-radius: 5px;
            padding: 4px 8px;
        }
        QGroupBox {
            border: 1px solid #2a2a2e;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 12px;
            font-weight: 600;
            color: #9a9aa0;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }
        QSlider::groove:horizontal { height: 4px; background: #2a2a2e; border-radius: 2px; }
        QSlider::handle:horizontal { background: #5ba8e6; width: 14px; margin: -5px 0; border-radius: 7px; }
        QScrollArea { border: none; }
    )"));

    buildUi();
    resize(480, 460);
}

void SettingsDialog::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);

    // --- General tab ---
    auto* generalTab = new QWidget(this);
    auto* generalLayout = new QVBoxLayout(generalTab);
    generalLayout->setContentsMargins(16, 16, 16, 16);
    generalLayout->setSpacing(12);

    auto* folderTitle = new QLabel(QStringLiteral("Default wallpaper folder"), this);
    generalLayout->addWidget(folderTitle);

    auto* folderRow = new QHBoxLayout();
    m_defaultFolderLabel = new QLabel(QStringLiteral("Not set"), this);
    m_defaultFolderLabel->setWordWrap(true);
    folderRow->addWidget(m_defaultFolderLabel, 1);
    auto* browseButton = new QPushButton(QStringLiteral("Browse..."), this);
    AnimationHelpers::installPressAnimation(browseButton);
    connect(browseButton, &QPushButton::clicked, this, [this] {
        QFileDialog::Options options;
#ifndef _WIN32
        // See the matching comment in SettingsWindow.cpp's "Open Folder"
        // handler - avoids depending on the XDG desktop portal.
        options |= QFileDialog::DontUseNativeDialog;
#endif
        const QString folder = QFileDialog::getExistingDirectory(
            this, QStringLiteral("Choose default wallpaper folder"), m_defaultFolder, options);
        if (!folder.isEmpty())
            emit defaultFolderChanged(folder);
    });
    folderRow->addWidget(browseButton);
    generalLayout->addLayout(folderRow);

    m_launchAtStartupCheck = new QCheckBox(QStringLiteral("Launch ChromaEngine at Windows startup"), this);
    connect(m_launchAtStartupCheck, &QCheckBox::toggled, this, &SettingsDialog::launchAtStartupToggled);
    generalLayout->addWidget(m_launchAtStartupCheck);

    m_closeToTrayCheck = new QCheckBox(QStringLiteral("Closing the window minimizes to the tray instead of exiting"),
                                        this);
    connect(m_closeToTrayCheck, &QCheckBox::toggled, this, &SettingsDialog::closeToTrayToggled);
    generalLayout->addWidget(m_closeToTrayCheck);

    m_startMinimizedCheck = new QCheckBox(QStringLiteral("Start minimized to the tray"), this);
    connect(m_startMinimizedCheck, &QCheckBox::toggled, this, &SettingsDialog::startMinimizedToggled);
    generalLayout->addWidget(m_startMinimizedCheck);

    m_thumbnailAutoPlayCheck =
        new QCheckBox(QStringLiteral("Auto-play animated previews in the library grid"), this);
    connect(m_thumbnailAutoPlayCheck, &QCheckBox::toggled, this, &SettingsDialog::thumbnailAutoPlayToggled);
    generalLayout->addWidget(m_thumbnailAutoPlayCheck);

    auto* memoryGroup = new QGroupBox(QStringLiteral("Memory usage"), this);
    auto* memoryLayout = new QVBoxLayout(memoryGroup);
    m_memoryLimitCheck = new QCheckBox(QStringLiteral("Limit this app's memory usage"), this);
    memoryLayout->addWidget(m_memoryLimitCheck);

    auto* memoryLimitRow = new QHBoxLayout();
    memoryLimitRow->addWidget(new QLabel(QStringLiteral("Limit:"), this));
    m_memoryLimitSpin = new QSpinBox(this);
    m_memoryLimitSpin->setRange(256, 32768);
    m_memoryLimitSpin->setSingleStep(256);
    m_memoryLimitSpin->setSuffix(QStringLiteral(" MB"));
    m_memoryLimitSpin->setValue(4096);
    memoryLimitRow->addWidget(m_memoryLimitSpin);
    memoryLimitRow->addStretch(1);
    memoryLayout->addLayout(memoryLimitRow);

    m_memoryLimitNote = new QLabel(
        QStringLiteral("The app is hard-capped at this limit and will be terminated by Windows if it's exceeded. "
                        "Takes effect the next time the app starts."),
        this);
    m_memoryLimitNote->setWordWrap(true);
    m_memoryLimitNote->setStyleSheet(QStringLiteral("color: #9a9aa0; font-size: 11px;"));
    memoryLayout->addWidget(m_memoryLimitNote);

    auto emitMemoryLimit = [this] { emit memoryLimitChanged(m_memoryLimitCheck->isChecked(), m_memoryLimitSpin->value()); };
    connect(m_memoryLimitCheck, &QCheckBox::toggled, this, emitMemoryLimit);
    connect(m_memoryLimitSpin, qOverload<int>(&QSpinBox::valueChanged), this, emitMemoryLimit);
    generalLayout->addWidget(memoryGroup);

    auto* renderingGroup = new QGroupBox(QStringLiteral("Rendering"), this);
    auto* renderingLayout = new QVBoxLayout(renderingGroup);
    m_softwareRenderingCheck =
        new QCheckBox(QStringLiteral("Force software rendering (compatibility mode)"), this);
    renderingLayout->addWidget(m_softwareRenderingCheck);

    auto* renderingNote = new QLabel(
        QStringLiteral("Turn this on if the wallpaper doesn't appear, or shows in its own separate window instead "
                        "of on the desktop - common on virtual machines and remote desktops without a real GPU. "
                        "Takes effect the next time the app starts."),
        this);
    renderingNote->setWordWrap(true);
    renderingNote->setStyleSheet(QStringLiteral("color: #9a9aa0; font-size: 11px;"));
    renderingLayout->addWidget(renderingNote);

    connect(m_softwareRenderingCheck, &QCheckBox::toggled, this, &SettingsDialog::softwareRenderingToggled);
    generalLayout->addWidget(renderingGroup);

    generalLayout->addStretch(1);
    tabs->addTab(generalTab, QStringLiteral("General"));

    // --- Displays tab ---
    auto* displaysTab = new QWidget(this);
    auto* displaysOuterLayout = new QVBoxLayout(displaysTab);
    displaysOuterLayout->setContentsMargins(16, 16, 16, 16);

    auto* hint = new QLabel(QStringLiteral("Choose which monitors show the wallpaper:"), this);
    hint->setWordWrap(true);
    displaysOuterLayout->addWidget(hint);

    m_displaysLayout = new QVBoxLayout();
    m_displaysLayout->setSpacing(8);
    displaysOuterLayout->addLayout(m_displaysLayout);
    displaysOuterLayout->addStretch(1);

    tabs->addTab(displaysTab, QStringLiteral("Displays"));

    // --- Appearance tab ---
    auto* appearanceTab = new QWidget(this);
    auto* appearanceLayout = new QVBoxLayout(appearanceTab);
    appearanceLayout->setContentsMargins(16, 16, 16, 16);
    appearanceLayout->setSpacing(12);

    auto* themeRow = new QHBoxLayout();
    themeRow->addWidget(new QLabel(QStringLiteral("Library background"), this));
    m_backgroundThemeCombo = new QComboBox(this);
    m_backgroundThemeCombo->addItem(QStringLiteral("None"));
    m_backgroundThemeCombo->addItem(QStringLiteral("Aurora"));
    m_backgroundThemeCombo->addItem(QStringLiteral("Starfield"));
    connect(m_backgroundThemeCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::backgroundThemeChanged);
    themeRow->addWidget(m_backgroundThemeCombo, 1);
    appearanceLayout->addLayout(themeRow);

    appearanceLayout->addStretch(1);
    tabs->addTab(appearanceTab, QStringLiteral("Appearance"));

    // --- Personalization tab ---
    tabs->addTab(buildPersonalizationTab(), QStringLiteral("Personalization"));

    layout->addWidget(tabs);
}

namespace {

QComboBox* makePositionCombo(QWidget* parent)
{
    auto* combo = new QComboBox(parent);
    combo->addItem(QStringLiteral("Top Left"), 0);
    combo->addItem(QStringLiteral("Top Right"), 1);
    combo->addItem(QStringLiteral("Bottom Left"), 2);
    combo->addItem(QStringLiteral("Bottom Right"), 3);
    combo->addItem(QStringLiteral("Center"), 4);
    return combo;
}

QComboBox* makeThemeCombo(QWidget* parent)
{
    auto* combo = new QComboBox(parent);
    combo->addItem(QStringLiteral("Light"), 0);
    combo->addItem(QStringLiteral("Dark"), 1);
    combo->addItem(QStringLiteral("Accent"), 2);
    combo->addItem(QStringLiteral("Outline"), 3);
    return combo;
}

QSlider* makeMarginSlider(QWidget* parent)
{
    auto* slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(0, 150);
    slider->setValue(40);
    return slider;
}

} // namespace

QWidget* SettingsDialog::buildPersonalizationTab()
{
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* page = new QWidget(this);
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(16, 16, 16, 16);
    pageLayout->setSpacing(14);

    // --- Clock ---
    auto* clockGroup = new QGroupBox(QStringLiteral("Clock"), this);
    auto* clockLayout = new QVBoxLayout(clockGroup);

    m_clockEnabledCheck = new QCheckBox(QStringLiteral("Show clock on the wallpaper"), this);
    clockLayout->addWidget(m_clockEnabledCheck);

    auto* clockLayoutRow = new QHBoxLayout();
    clockLayoutRow->addWidget(new QLabel(QStringLiteral("Layout"), this));
    m_clockLayoutCombo = new QComboBox(this);
    m_clockLayoutCombo->addItem(QStringLiteral("Digital"), 0);
    m_clockLayoutCombo->addItem(QStringLiteral("Digital + Date"), 1);
    m_clockLayoutCombo->addItem(QStringLiteral("Analog"), 2);
    clockLayoutRow->addWidget(m_clockLayoutCombo, 1);
    clockLayout->addLayout(clockLayoutRow);

    auto* clockThemeRow = new QHBoxLayout();
    clockThemeRow->addWidget(new QLabel(QStringLiteral("Theme"), this));
    m_clockThemeCombo = makeThemeCombo(this);
    clockThemeRow->addWidget(m_clockThemeCombo, 1);
    clockLayout->addLayout(clockThemeRow);

    auto* clockFontRow = new QHBoxLayout();
    clockFontRow->addWidget(new QLabel(QStringLiteral("Font"), this));
    m_clockFontCombo = new QFontComboBox(this);
    clockFontRow->addWidget(m_clockFontCombo, 1);
    clockLayout->addLayout(clockFontRow);

    auto* clockSizeRow = new QHBoxLayout();
    clockSizeRow->addWidget(new QLabel(QStringLiteral("Size"), this));
    m_clockFontSizeSlider = new QSlider(Qt::Horizontal, this);
    m_clockFontSizeSlider->setRange(16, 120);
    m_clockFontSizeSlider->setValue(48);
    clockSizeRow->addWidget(m_clockFontSizeSlider, 1);
    clockLayout->addLayout(clockSizeRow);

    auto* clockRotationRow = new QHBoxLayout();
    clockRotationRow->addWidget(new QLabel(QStringLiteral("Orientation"), this));
    m_clockRotationCombo = new QComboBox(this);
    m_clockRotationCombo->addItem(QStringLiteral("Normal"), 0);
    m_clockRotationCombo->addItem(QStringLiteral("Rotated 90°"), 90);
    m_clockRotationCombo->addItem(QStringLiteral("Upside down"), 180);
    m_clockRotationCombo->addItem(QStringLiteral("Rotated 270°"), 270);
    clockRotationRow->addWidget(m_clockRotationCombo, 1);
    clockLayout->addLayout(clockRotationRow);

    auto* clockPositionRow = new QHBoxLayout();
    clockPositionRow->addWidget(new QLabel(QStringLiteral("Location"), this));
    m_clockPositionCombo = makePositionCombo(this);
    clockPositionRow->addWidget(m_clockPositionCombo, 1);
    clockLayout->addLayout(clockPositionRow);

    auto* clockMarginRow = new QHBoxLayout();
    clockMarginRow->addWidget(new QLabel(QStringLiteral("Margin"), this));
    m_clockMarginSlider = makeMarginSlider(this);
    clockMarginRow->addWidget(m_clockMarginSlider, 1);
    clockLayout->addLayout(clockMarginRow);

    pageLayout->addWidget(clockGroup);

    for (QCheckBox* box : {m_clockEnabledCheck})
        connect(box, &QCheckBox::toggled, this, &SettingsDialog::emitClockSettings);
    connect(m_clockLayoutCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::emitClockSettings);
    connect(m_clockThemeCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::emitClockSettings);
    connect(m_clockFontCombo, &QFontComboBox::currentFontChanged, this, &SettingsDialog::emitClockSettings);
    connect(m_clockFontSizeSlider, &QSlider::valueChanged, this, &SettingsDialog::emitClockSettings);
    connect(m_clockRotationCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::emitClockSettings);
    connect(m_clockPositionCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::emitClockSettings);
    connect(m_clockMarginSlider, &QSlider::valueChanged, this, &SettingsDialog::emitClockSettings);

    // --- Calendar ---
    auto* calendarGroup = new QGroupBox(QStringLiteral("Calendar"), this);
    auto* calendarLayout = new QVBoxLayout(calendarGroup);

    m_calendarEnabledCheck = new QCheckBox(QStringLiteral("Show calendar on the wallpaper"), this);
    calendarLayout->addWidget(m_calendarEnabledCheck);

    auto* calendarThemeRow = new QHBoxLayout();
    calendarThemeRow->addWidget(new QLabel(QStringLiteral("Theme"), this));
    m_calendarThemeCombo = makeThemeCombo(this);
    calendarThemeRow->addWidget(m_calendarThemeCombo, 1);
    calendarLayout->addLayout(calendarThemeRow);

    auto* calendarPositionRow = new QHBoxLayout();
    calendarPositionRow->addWidget(new QLabel(QStringLiteral("Location"), this));
    m_calendarPositionCombo = makePositionCombo(this);
    calendarPositionRow->addWidget(m_calendarPositionCombo, 1);
    calendarLayout->addLayout(calendarPositionRow);

    auto* calendarMarginRow = new QHBoxLayout();
    calendarMarginRow->addWidget(new QLabel(QStringLiteral("Margin"), this));
    m_calendarMarginSlider = makeMarginSlider(this);
    calendarMarginRow->addWidget(m_calendarMarginSlider, 1);
    calendarLayout->addLayout(calendarMarginRow);

    pageLayout->addWidget(calendarGroup);

    connect(m_calendarEnabledCheck, &QCheckBox::toggled, this, &SettingsDialog::emitCalendarSettings);
    connect(m_calendarThemeCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::emitCalendarSettings);
    connect(m_calendarPositionCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::emitCalendarSettings);
    connect(m_calendarMarginSlider, &QSlider::valueChanged, this, &SettingsDialog::emitCalendarSettings);

    // --- Bluetooth battery ---
    auto* batteryGroup = new QGroupBox(QStringLiteral("Bluetooth Battery Indicator"), this);
    auto* batteryLayout = new QVBoxLayout(batteryGroup);

    m_batteryEnabledCheck = new QCheckBox(QStringLiteral("Show battery levels of connected Bluetooth devices"), this);
    batteryLayout->addWidget(m_batteryEnabledCheck);

    auto* batteryThemeRow = new QHBoxLayout();
    batteryThemeRow->addWidget(new QLabel(QStringLiteral("Theme"), this));
    m_batteryThemeCombo = makeThemeCombo(this);
    batteryThemeRow->addWidget(m_batteryThemeCombo, 1);
    batteryLayout->addLayout(batteryThemeRow);

    auto* batteryPositionRow = new QHBoxLayout();
    batteryPositionRow->addWidget(new QLabel(QStringLiteral("Location"), this));
    m_batteryPositionCombo = makePositionCombo(this);
    batteryPositionRow->addWidget(m_batteryPositionCombo, 1);
    batteryLayout->addLayout(batteryPositionRow);

    auto* batteryMarginRow = new QHBoxLayout();
    batteryMarginRow->addWidget(new QLabel(QStringLiteral("Margin"), this));
    m_batteryMarginSlider = makeMarginSlider(this);
    batteryMarginRow->addWidget(m_batteryMarginSlider, 1);
    batteryLayout->addLayout(batteryMarginRow);

    pageLayout->addWidget(batteryGroup);

    connect(m_batteryEnabledCheck, &QCheckBox::toggled, this, &SettingsDialog::emitBatterySettings);
    connect(m_batteryThemeCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::emitBatterySettings);
    connect(m_batteryPositionCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::emitBatterySettings);
    connect(m_batteryMarginSlider, &QSlider::valueChanged, this, &SettingsDialog::emitBatterySettings);

    pageLayout->addStretch(1);
    scrollArea->setWidget(page);
    return scrollArea;
}

void SettingsDialog::emitClockSettings()
{
    emit clockSettingsChanged(m_clockEnabledCheck->isChecked(), m_clockLayoutCombo->currentData().toInt(),
                               m_clockThemeCombo->currentData().toInt(), m_clockFontCombo->currentFont().family(),
                               m_clockFontSizeSlider->value(), m_clockRotationCombo->currentData().toInt(),
                               m_clockPositionCombo->currentData().toInt(), m_clockMarginSlider->value());
}

void SettingsDialog::emitCalendarSettings()
{
    emit calendarSettingsChanged(m_calendarEnabledCheck->isChecked(), m_calendarThemeCombo->currentData().toInt(),
                                  m_calendarPositionCombo->currentData().toInt(), m_calendarMarginSlider->value());
}

void SettingsDialog::emitBatterySettings()
{
    emit batterySettingsChanged(m_batteryEnabledCheck->isChecked(), m_batteryThemeCombo->currentData().toInt(),
                                 m_batteryPositionCombo->currentData().toInt(), m_batteryMarginSlider->value());
}

void SettingsDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    AnimationHelpers::fadeIn(this);
}

void SettingsDialog::setLaunchAtStartup(bool enabled)
{
    const QSignalBlocker blocker(m_launchAtStartupCheck);
    m_launchAtStartupCheck->setChecked(enabled);
}

void SettingsDialog::setCloseToTray(bool enabled)
{
    const QSignalBlocker blocker(m_closeToTrayCheck);
    m_closeToTrayCheck->setChecked(enabled);
}

void SettingsDialog::setStartMinimized(bool enabled)
{
    const QSignalBlocker blocker(m_startMinimizedCheck);
    m_startMinimizedCheck->setChecked(enabled);
}

void SettingsDialog::setDefaultFolder(const QString& folder)
{
    m_defaultFolder = folder;
    m_defaultFolderLabel->setText(folder.isEmpty() ? QStringLiteral("Not set") : folder);
}

void SettingsDialog::setThumbnailAutoPlay(bool enabled)
{
    const QSignalBlocker blocker(m_thumbnailAutoPlayCheck);
    m_thumbnailAutoPlayCheck->setChecked(enabled);
}

void SettingsDialog::setMemoryLimit(bool enabled, int limitMb)
{
    const QSignalBlocker checkBlocker(m_memoryLimitCheck);
    const QSignalBlocker spinBlocker(m_memoryLimitSpin);
    m_memoryLimitCheck->setChecked(enabled);
    m_memoryLimitSpin->setValue(limitMb);
}

void SettingsDialog::setSoftwareRendering(bool enabled)
{
    const QSignalBlocker blocker(m_softwareRenderingCheck);
    m_softwareRenderingCheck->setChecked(enabled);
}

void SettingsDialog::setBackgroundTheme(int themeIndex)
{
    const QSignalBlocker blocker(m_backgroundThemeCombo);
    m_backgroundThemeCombo->setCurrentIndex(themeIndex);
}

void SettingsDialog::setClockSettings(bool enabled, int layout, int theme, const QString& fontFamily, int fontSize,
                                       int rotation, int position, int margin)
{
    const QSignalBlocker b1(m_clockEnabledCheck);
    const QSignalBlocker b2(m_clockLayoutCombo);
    const QSignalBlocker b3(m_clockThemeCombo);
    const QSignalBlocker b4(m_clockFontCombo);
    const QSignalBlocker b5(m_clockFontSizeSlider);
    const QSignalBlocker b6(m_clockRotationCombo);
    const QSignalBlocker b7(m_clockPositionCombo);
    const QSignalBlocker b8(m_clockMarginSlider);

    m_clockEnabledCheck->setChecked(enabled);
    m_clockLayoutCombo->setCurrentIndex(m_clockLayoutCombo->findData(layout));
    m_clockThemeCombo->setCurrentIndex(m_clockThemeCombo->findData(theme));
    if (!fontFamily.isEmpty())
        m_clockFontCombo->setCurrentFont(QFont(fontFamily));
    m_clockFontSizeSlider->setValue(fontSize);
    m_clockRotationCombo->setCurrentIndex(m_clockRotationCombo->findData(rotation));
    m_clockPositionCombo->setCurrentIndex(m_clockPositionCombo->findData(position));
    m_clockMarginSlider->setValue(margin);
}

void SettingsDialog::setCalendarSettings(bool enabled, int theme, int position, int margin)
{
    const QSignalBlocker b1(m_calendarEnabledCheck);
    const QSignalBlocker b2(m_calendarThemeCombo);
    const QSignalBlocker b3(m_calendarPositionCombo);
    const QSignalBlocker b4(m_calendarMarginSlider);

    m_calendarEnabledCheck->setChecked(enabled);
    m_calendarThemeCombo->setCurrentIndex(m_calendarThemeCombo->findData(theme));
    m_calendarPositionCombo->setCurrentIndex(m_calendarPositionCombo->findData(position));
    m_calendarMarginSlider->setValue(margin);
}

void SettingsDialog::setBatterySettings(bool enabled, int theme, int position, int margin)
{
    const QSignalBlocker b1(m_batteryEnabledCheck);
    const QSignalBlocker b2(m_batteryThemeCombo);
    const QSignalBlocker b3(m_batteryPositionCombo);
    const QSignalBlocker b4(m_batteryMarginSlider);

    m_batteryEnabledCheck->setChecked(enabled);
    m_batteryThemeCombo->setCurrentIndex(m_batteryThemeCombo->findData(theme));
    m_batteryPositionCombo->setCurrentIndex(m_batteryPositionCombo->findData(position));
    m_batteryMarginSlider->setValue(margin);
}

void SettingsDialog::setMonitors(const QList<MonitorInfo>& monitors, const QStringList& enabledIds)
{
    QLayoutItem* item = nullptr;
    while ((item = m_displaysLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // Empty enabledIds means "primary only" (the original default), not
    // "nothing" - reflect that in the checkboxes rather than showing every
    // monitor unchecked.
    for (const MonitorInfo& monitor : monitors) {
        const bool enabled = enabledIds.isEmpty() ? monitor.isPrimary : enabledIds.contains(monitor.id);

        auto* checkBox = new QCheckBox(monitor.name, this);
        checkBox->setChecked(enabled);
        const QString id = monitor.id;
        connect(checkBox, &QCheckBox::toggled, this,
                [this, id](bool checked) { emit monitorToggled(id, checked); });
        m_displaysLayout->addWidget(checkBox);
    }
}

} // namespace colorfy
