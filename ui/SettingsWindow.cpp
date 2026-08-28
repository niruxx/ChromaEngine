#include "SettingsWindow.h"

#include <QAbstractButton>
#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QResizeEvent>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSlider>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStorageInfo>
#include <QStyle>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include <cmath>

#include "AnimatedBackground.h"
#include "AnimationHelpers.h"
#include "CustomTitleBar.h"
#include "IconFactory.h"
#include "LibraryItemDelegate.h"
#include "SettingsDialog.h"
#include "colorfy/LibraryScanner.h"

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#endif

namespace colorfy {

namespace {

constexpr int kThumbnailWidth = 160;
constexpr int kThumbnailHeight = 90;
constexpr int kFrameAdvanceMs = 220;

QPixmap scaledThumbnail(const QPixmap& source)
{
    return source.scaled(kThumbnailWidth, kThumbnailHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
}

QString humanFileSize(qint64 bytes)
{
    const QStringList units = {QStringLiteral("B"), QStringLiteral("KB"), QStringLiteral("MB"),
                                QStringLiteral("GB")};
    double size = bytes;
    int unitIndex = 0;
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        ++unitIndex;
    }
    const int precision = (size < 10 && unitIndex > 0) ? 1 : 0;
    return QStringLiteral("%1 %2").arg(size, 0, 'f', precision).arg(units.at(unitIndex));
}

QSlider* makeSlider(int min, int max, int value)
{
    auto* slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(value);
    return slider;
}

// TEMPORARY diagnostic: pinpointing a reported freeze (spinning cursor,
// rest of the OS stays responsive) when closing to tray then right-
// clicking the tray icon. Shares one log file with TrayIcon.cpp and
// main_windows.cpp's heartbeat so the whole sequence interleaves into one
// timeline. Remove once resolved.
void chromaDebugLog(const QString& line)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(dir);
    QFile f(dir + QStringLiteral("/chroma_debug.log"));
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " " << line << Qt::endl;
    }
}

} // namespace

SettingsWindow::SettingsWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("ChromaEngine"));
    setWindowFlag(Qt::FramelessWindowHint, true);
    setStyleSheet(QStringLiteral(R"(
        QWidget#libraryPanel, QWidget#previewPanel { background: transparent; }
        QWidget { color: #e6e6e6; }
        QToolBar {
            background-color: rgba(28, 28, 32, 190);
            border: none;
            border-bottom: 1px solid #2a2a2e;
            padding: 6px;
            spacing: 4px;
        }
        QToolButton {
            background-color: transparent;
            border-radius: 6px;
            padding: 6px 10px;
        }
        QToolButton:hover { background-color: #2a2a30; }
        QToolButton:checked { background-color: #2f527a; }
        QStatusBar {
            background-color: rgba(28, 28, 32, 190);
            border-top: 1px solid #2a2a2e;
            color: #9a9aa0;
        }
        QLabel#sectionTitle {
            color: #8a8a92;
            font-size: 11px;
            font-weight: 600;
            letter-spacing: 1px;
            padding-top: 10px;
            padding-bottom: 4px;
        }
        QLabel#fileMeta { color: #8a8a92; font-size: 11px; }
        QListWidget {
            background-color: transparent;
            border: none;
            outline: none;
        }
        QFrame#previewFrame {
            background-color: #000000;
            border: 1px solid #2a2a2e;
            border-radius: 8px;
        }
        QLabel#previewFilename {
            color: #c8c8ce;
            font-weight: 500;
            padding-top: 6px;
        }
        QPushButton {
            background-color: #24242a;
            border: 1px solid #34343c;
            border-radius: 6px;
            padding: 6px 10px;
        }
        QPushButton:hover { background-color: #2b2b32; }
        QPushButton#setWallpaperButton {
            background-color: #2f6fae;
            border-color: #3a7fc2;
            font-weight: 600;
        }
        QPushButton#setWallpaperButton:hover { background-color: #3a7fc2; }
        QCheckBox::indicator {
            width: 16px; height: 16px;
            border-radius: 4px;
            border: 1px solid #4a4a52;
            background-color: #222226;
        }
        QCheckBox::indicator:checked { background-color: #5ba8e6; border-color: #5ba8e6; }
        QSlider::groove:horizontal {
            height: 4px;
            background: #2a2a2e;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #5ba8e6;
            width: 14px;
            margin: -5px 0;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:pressed { background: #7ec1f5; }
        QComboBox {
            background-color: #222226;
            border: 1px solid #3a3a40;
            border-radius: 5px;
            padding: 4px 8px;
        }
        QScrollArea { border: none; background: transparent; }
        QScrollArea > QWidget { background: transparent; }
        QSplitter::handle {
            background-color: rgba(23, 23, 26, 140);
            width: 1px;
        }
    )"));

    buildUi();
    resize(1040, 680);
#ifndef _WIN32
    // Frameless top-level windows don't reliably get centered by the window
    // manager the way decorated ones do (confirmed live on xfwm4: it just
    // appears wherever the WM's default placement happens to land it,
    // nowhere near the screen center) - Windows' own placement already
    // centers acceptably without this, so it's scoped to here rather than
    // risking a behavior change on the platform that's actually been tested.
    if (QScreen* activeScreen = screen()) {
        const QRect available = activeScreen->availableGeometry();
        move(available.center() - QPoint(width() / 2, height() / 2));
    }
#endif

    m_frameTimer = new QTimer(this);
    connect(m_frameTimer, &QTimer::timeout, this, &SettingsWindow::advanceFrame);
}

void SettingsWindow::buildUi()
{
    m_titleBar = new CustomTitleBar(windowTitle(), this);
    connect(m_titleBar, &CustomTitleBar::minimizeRequested, this, &SettingsWindow::showMinimized);
    connect(m_titleBar, &CustomTitleBar::maximizeRestoreRequested, this, [this] {
        if (isMaximized())
            showNormal();
        else
            showMaximized();
    });
    connect(m_titleBar, &CustomTitleBar::closeRequested, this, &SettingsWindow::close);
    // Repainting an animated, window-spanning background (and, if auto-play
    // is on, cycling every library tile's frame) on every drag-move frame
    // competed with the move itself for repaint bandwidth and made
    // dragging feel laggy. Silencing both for the duration of a drag is
    // imperceptible (nobody's looking at the background animate mid-drag)
    // and removes that contention. m_animatedBackground/m_frameTimer aren't
    // created yet at this point in buildUi(), but these lambdas only run
    // later, once an actual drag happens.
    connect(m_titleBar, &CustomTitleBar::dragStarted, this, [this] {
        m_animatedBackground->setPaused(true);
        m_frameTimer->stop();
    });
    connect(m_titleBar, &CustomTitleBar::dragFinished, this, [this] {
        m_animatedBackground->setPaused(false);
        if (m_thumbnailAutoPlayEnabled)
            m_frameTimer->start(kFrameAdvanceMs);
    });
    setMenuWidget(m_titleBar);

    auto* toolbar = addToolBar(QStringLiteral("Main"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setIconSize(QSize(18, 18));

    auto* openFolderAction = toolbar->addAction(IconFactory::folder(), QStringLiteral("Open Folder"));
    connect(openFolderAction, &QAction::triggered, this, [this] {
        QFileDialog::Options options;
#ifndef _WIN32
        // Qt6's native dialog on Linux goes through the XDG desktop portal
        // (org.freedesktop.portal.FileChooser) - on a system with no portal
        // backend running (confirmed live: no session/desktop portal
        // service available), that call fails and getExistingDirectory()
        // silently returns an empty string with no dialog ever shown at
        // all, rather than falling back automatically. Forcing Qt's own
        // built-in dialog sidesteps the portal entirely.
        options |= QFileDialog::DontUseNativeDialog;
#endif
        const QString folder = QFileDialog::getExistingDirectory(
            this, QStringLiteral("Choose wallpaper folder"), m_currentFolder, options);
        if (!folder.isEmpty())
            emit folderChanged(folder);
    });

    auto* refreshAction = toolbar->addAction(IconFactory::refresh(), QStringLiteral("Refresh"));
    connect(refreshAction, &QAction::triggered, this, &SettingsWindow::refreshRequested);

    toolbar->addSeparator();

    m_playPauseAction = toolbar->addAction(IconFactory::pause(), QStringLiteral("Pause"));
    m_playPauseAction->setCheckable(true);
    connect(m_playPauseAction, &QAction::toggled, this, [this](bool paused) {
        m_playPauseAction->setText(paused ? QStringLiteral("Play") : QStringLiteral("Pause"));
        m_playPauseAction->setIcon(paused ? IconFactory::play() : IconFactory::pause());
        emit playPauseToggled(paused);
    });

    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    m_settingsDialog = new SettingsDialog(this);
    connect(m_settingsDialog, &SettingsDialog::backgroundThemeChanged, this,
            [this](int theme) { m_animatedBackground->setTheme(theme); });
    auto* settingsAction = toolbar->addAction(IconFactory::gear(), QStringLiteral("Settings"));
    connect(settingsAction, &QAction::triggered, this, [this] {
        m_settingsDialog->show();
        m_settingsDialog->raise();
        m_settingsDialog->activateWindow();
    });

    for (QAction* action : toolbar->actions()) {
        if (QWidget* w = toolbar->widgetForAction(action)) {
            if (auto* button = qobject_cast<QAbstractButton*>(w))
                AnimationHelpers::installPressAnimation(button);
        }
    }

    // Backdrop for the whole window, not just one panel: a plain child of
    // this QMainWindow (outside setMenuWidget/addToolBar/setCentralWidget's
    // own layout, so it's exempt from QMainWindowLayout entirely), manually
    // kept full-window-sized in resizeEvent() and lowered behind every other
    // child. The title bar/toolbar/panels/status bar all use semi-
    // transparent backgrounds so it shows through everywhere, not just in
    // the gaps between them.
    m_animatedBackground = new AnimatedBackground(this);
    m_animatedBackground->setGeometry(rect());
    m_animatedBackground->lower();

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setStyleSheet(QStringLiteral("QSplitter { background: transparent; }"));

    // --- Library panel -----------------------------------------------
    auto* libraryPanel = new QWidget(this);
    libraryPanel->setObjectName(QStringLiteral("libraryPanel"));
    auto* libraryLayout = new QVBoxLayout(libraryPanel);
    libraryLayout->setContentsMargins(14, 12, 6, 12);

    auto* libraryTitle = new QLabel(QStringLiteral("LIBRARY"), this);
    libraryTitle->setObjectName(QStringLiteral("sectionTitle"));
    libraryLayout->addWidget(libraryTitle);

    m_libraryGrid = new QListWidget(this);
    m_libraryGrid->setViewMode(QListView::IconMode);
    m_libraryGrid->setIconSize(QSize(kThumbnailWidth, kThumbnailHeight));
    m_libraryGrid->setGridSize(QSize(184, 136));
    m_libraryGrid->setResizeMode(QListView::Adjust);
    m_libraryGrid->setMovement(QListView::Static);
    m_libraryGrid->setSpacing(2);
    m_libraryGrid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_delegate = new LibraryItemDelegate(m_libraryGrid);
    m_libraryGrid->setItemDelegate(m_delegate);
    m_libraryGrid->setFrameShape(QFrame::NoFrame);
    connect(m_libraryGrid, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        const QString path = item->data(Qt::UserRole).toString();
        m_previewedPath = path;
        m_previewFilenameLabel->setText(QFileInfo(path).fileName());
        updateFileInfo(path);
        emit mediaPreviewed(path);
    });
    libraryLayout->addWidget(m_libraryGrid, 1);

    splitter->addWidget(libraryPanel);

    // --- Preview panel (scrollable: a lot of controls live here) -------
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumWidth(320);
    scrollArea->setMaximumWidth(380);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* previewPanel = new QWidget(this);
    previewPanel->setObjectName(QStringLiteral("previewPanel"));
    auto* previewLayout = new QVBoxLayout(previewPanel);
    previewLayout->setContentsMargins(10, 12, 14, 12);

    auto* previewTitle = new QLabel(QStringLiteral("PREVIEW"), this);
    previewTitle->setObjectName(QStringLiteral("sectionTitle"));
    previewLayout->addWidget(previewTitle);

    auto* previewFrame = new QFrame(this);
    previewFrame->setObjectName(QStringLiteral("previewFrame"));
    previewFrame->setFixedHeight(190);
    auto* previewFrameLayout = new QVBoxLayout(previewFrame);
    previewFrameLayout->setContentsMargins(1, 1, 1, 1);

    m_previewWidget = new QWidget(previewFrame);
    m_previewWidget->setAttribute(Qt::WA_NativeWindow);
    m_previewWidget->setAutoFillBackground(true);
    QPalette pal = m_previewWidget->palette();
    pal.setColor(QPalette::Window, Qt::black);
    m_previewWidget->setPalette(pal);
    previewFrameLayout->addWidget(m_previewWidget);

    previewLayout->addWidget(previewFrame);

    m_previewFilenameLabel = new QLabel(QStringLiteral("No wallpaper selected"), this);
    m_previewFilenameLabel->setObjectName(QStringLiteral("previewFilename"));
    m_previewFilenameLabel->setWordWrap(true);
    previewLayout->addWidget(m_previewFilenameLabel);

    auto* metaRow = new QHBoxLayout();
    m_fileSizeLabel = new QLabel(QStringLiteral("—"), this);
    m_fileSizeLabel->setObjectName(QStringLiteral("fileMeta"));
    m_fileResolutionLabel = new QLabel(QStringLiteral("—"), this);
    m_fileResolutionLabel->setObjectName(QStringLiteral("fileMeta"));
    metaRow->addWidget(m_fileSizeLabel);
    metaRow->addStretch(1);
    metaRow->addWidget(m_fileResolutionLabel);
    previewLayout->addLayout(metaRow);

    auto* actionsRow = new QHBoxLayout();
    m_renameButton = new QPushButton(IconFactory::rename(16), QStringLiteral("Rename"), this);
    m_deleteButton = new QPushButton(IconFactory::trash(16), QStringLiteral("Delete"), this);
    AnimationHelpers::installPressAnimation(m_renameButton);
    AnimationHelpers::installPressAnimation(m_deleteButton);
    connect(m_renameButton, &QPushButton::clicked, this, [this] {
        if (m_previewedPath.isEmpty())
            return;
        const QFileInfo info(m_previewedPath);
        bool ok = false;
        const QString newName = QInputDialog::getText(this, QStringLiteral("Rename"), QStringLiteral("New name:"),
                                                        QLineEdit::Normal, info.fileName(), &ok);
        if (ok && !newName.isEmpty() && newName != info.fileName())
            emit renameRequested(m_previewedPath, newName);
    });
    connect(m_deleteButton, &QPushButton::clicked, this, [this] {
        if (m_previewedPath.isEmpty())
            return;
        const QFileInfo info(m_previewedPath);
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Delete wallpaper"),
            QStringLiteral("Permanently delete \"%1\"? This cannot be undone.").arg(info.fileName()));
        if (answer == QMessageBox::Yes)
            emit deleteRequested(m_previewedPath);
    });
    actionsRow->addWidget(m_renameButton);
    actionsRow->addWidget(m_deleteButton);
    previewLayout->addLayout(actionsRow);

    m_setWallpaperButton = new QPushButton(IconFactory::applyCheck(16), QStringLiteral("Set as Wallpaper"), this);
    m_setWallpaperButton->setObjectName(QStringLiteral("setWallpaperButton"));
    AnimationHelpers::installPressAnimation(m_setWallpaperButton);
    connect(m_setWallpaperButton, &QPushButton::clicked, this, [this] {
        if (!m_previewedPath.isEmpty())
            emit setWallpaperRequested(m_previewedPath);
    });
    previewLayout->addWidget(m_setWallpaperButton);

    // --- Playback ---
    auto* playbackTitle = new QLabel(QStringLiteral("PLAYBACK"), this);
    playbackTitle->setObjectName(QStringLiteral("sectionTitle"));
    previewLayout->addWidget(playbackTitle);

    auto* muteRow = new QHBoxLayout();
    m_muteCheckBox = new QCheckBox(QStringLiteral("Mute"), this);
    m_muteCheckBox->setChecked(true);
    connect(m_muteCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_volumeSlider->setEnabled(!checked);
        emit mutedChanged(checked);
    });
    muteRow->addWidget(m_muteCheckBox);
    m_volumeSlider = makeSlider(0, 100, 100);
    m_volumeSlider->setEnabled(false);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &SettingsWindow::volumeChanged);
    muteRow->addWidget(m_volumeSlider, 1);
    previewLayout->addLayout(muteRow);

    auto* speedRow = new QHBoxLayout();
    speedRow->addWidget(new QLabel(QStringLiteral("Speed"), this));
    m_speedSlider = makeSlider(25, 300, 100);
    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int value) {
        const double rate = value / 100.0;
        m_speedLabel->setText(QStringLiteral("%1x").arg(rate, 0, 'f', 2));
        emit speedChanged(rate);
    });
    speedRow->addWidget(m_speedSlider, 1);
    m_speedLabel = new QLabel(QStringLiteral("1.00x"), this);
    m_speedLabel->setFixedWidth(40);
    speedRow->addWidget(m_speedLabel);
    previewLayout->addLayout(speedRow);

    auto* frameRateRow = new QHBoxLayout();
    frameRateRow->addWidget(new QLabel(QStringLiteral("Preview frame rate"), this));
    m_frameRateLimitCombo = new QComboBox(this);
    m_frameRateLimitCombo->addItem(QStringLiteral("Unlimited"), 0);
    m_frameRateLimitCombo->addItem(QStringLiteral("15 fps"), 15);
    m_frameRateLimitCombo->addItem(QStringLiteral("30 fps"), 30);
    m_frameRateLimitCombo->addItem(QStringLiteral("60 fps"), 60);
    connect(m_frameRateLimitCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        emit previewFrameRateLimitChanged(m_frameRateLimitCombo->itemData(index).toInt());
    });
    frameRateRow->addWidget(m_frameRateLimitCombo, 1);
    previewLayout->addLayout(frameRateRow);

    // --- Appearance ---
    auto* appearanceTitle = new QLabel(QStringLiteral("APPEARANCE"), this);
    appearanceTitle->setObjectName(QStringLiteral("sectionTitle"));
    previewLayout->addWidget(appearanceTitle);

    auto* fitRow = new QHBoxLayout();
    fitRow->addWidget(new QLabel(QStringLiteral("Alignment"), this));
    m_fitModeCombo = new QComboBox(this);
    m_fitModeCombo->addItem(QStringLiteral("Fill (Cover)"), static_cast<int>(FitMode::Fill));
    m_fitModeCombo->addItem(QStringLiteral("Fit"), static_cast<int>(FitMode::Fit));
    m_fitModeCombo->addItem(QStringLiteral("Stretch"), static_cast<int>(FitMode::Stretch));
    m_fitModeCombo->addItem(QStringLiteral("Center"), static_cast<int>(FitMode::Center));
    m_fitModeCombo->addItem(QStringLiteral("Free (Zoom)"), static_cast<int>(FitMode::Free));
    connect(m_fitModeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const auto mode = static_cast<FitMode>(m_fitModeCombo->itemData(index).toInt());
        const bool isFree = mode == FitMode::Free;
        m_zoomRow->setVisible(isFree);
        emit fitModeChanged(mode);
    });
    fitRow->addWidget(m_fitModeCombo, 1);
    previewLayout->addLayout(fitRow);

    m_zoomRow = new QWidget(this);
    auto* zoomRow = new QHBoxLayout(m_zoomRow);
    zoomRow->setContentsMargins(0, 0, 0, 0);
    zoomRow->addWidget(new QLabel(QStringLiteral("Zoom"), this));
    m_zoomSlider = makeSlider(-100, 100, 0);
    connect(m_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        const double logScale = value / 100.0;
        m_zoomLabel->setText(QStringLiteral("%1%").arg(static_cast<int>(std::pow(2.0, logScale) * 100)));
        emit zoomChanged(logScale);
    });
    zoomRow->addWidget(m_zoomSlider, 1);
    m_zoomLabel = new QLabel(QStringLiteral("100%"), this);
    m_zoomLabel->setFixedWidth(40);
    zoomRow->addWidget(m_zoomLabel);
    m_zoomRow->setVisible(false);
    previewLayout->addWidget(m_zoomRow);

    auto* flipRow = new QHBoxLayout();
    m_flipHCheckBox = new QCheckBox(QStringLiteral("Flip H"), this);
    m_flipVCheckBox = new QCheckBox(QStringLiteral("Flip V"), this);
    auto emitFlip = [this] { emit flipChanged(m_flipHCheckBox->isChecked(), m_flipVCheckBox->isChecked()); };
    connect(m_flipHCheckBox, &QCheckBox::toggled, this, emitFlip);
    connect(m_flipVCheckBox, &QCheckBox::toggled, this, emitFlip);
    flipRow->addWidget(m_flipHCheckBox);
    flipRow->addWidget(m_flipVCheckBox);
    flipRow->addStretch(1);
    previewLayout->addLayout(flipRow);

    m_showDesktopIconsCheckBox = new QCheckBox(QStringLiteral("Show desktop icons"), this);
    m_showDesktopIconsCheckBox->setChecked(true);
    m_showDesktopIconsCheckBox->setToolTip(
        QStringLiteral("When off, the wallpaper covers desktop icons instead of sitting behind them."));
    connect(m_showDesktopIconsCheckBox, &QCheckBox::toggled, this, &SettingsWindow::showDesktopIconsChanged);
    previewLayout->addWidget(m_showDesktopIconsCheckBox);
#ifndef _WIN32
    // No cross-desktop-environment way to reorder against XFCE's/MATE's own
    // desktop-icon window the way the Windows WorkerW-based build can (see
    // platform/linux/main_linux.cpp) - hidden rather than left as a control
    // that silently does nothing.
    m_showDesktopIconsCheckBox->hide();
#endif

    auto addFilterRow = [this, &previewLayout](const QString& label, QSlider*& sliderOut, auto&& onChange) {
        auto* row = new QHBoxLayout();
        row->addWidget(new QLabel(label, this));
        sliderOut = makeSlider(-100, 100, 0);
        connect(sliderOut, &QSlider::valueChanged, this, onChange);
        row->addWidget(sliderOut, 1);
        previewLayout->addLayout(row);
    };
    addFilterRow(QStringLiteral("Brightness"), m_brightnessSlider,
                 [this](int v) { emit brightnessChanged(v); });
    addFilterRow(QStringLiteral("Contrast"), m_contrastSlider, [this](int v) { emit contrastChanged(v); });
    addFilterRow(QStringLiteral("Saturation"), m_saturationSlider,
                 [this](int v) { emit saturationChanged(v); });

    previewLayout->addStretch(1);

    scrollArea->setWidget(previewPanel);
    splitter->addWidget(scrollArea);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);

    setCentralWidget(splitter);

    m_countLabel = new QLabel(QStringLiteral("No folder selected"), this);
    statusBar()->addWidget(m_countLabel);

    m_storageLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_storageLabel);
    updateStorageLabel();

    // Free space changes over time as files are added/removed elsewhere;
    // keep the readout reasonably current without polling aggressively.
    auto* storageTimer = new QTimer(this);
    connect(storageTimer, &QTimer::timeout, this, &SettingsWindow::updateStorageLabel);
    storageTimer->start(60000);
}

void SettingsWindow::setThumbnailAutoPlayEnabled(bool enabled)
{
    m_thumbnailAutoPlayEnabled = enabled;
    if (enabled) {
        m_frameTimer->start(kFrameAdvanceMs);
        return;
    }

    m_frameTimer->stop();
    m_currentFrame = 0;
    m_delegate->setCurrentFrame(0);
    if (m_libraryGrid->viewport())
        m_libraryGrid->viewport()->update();
}

void SettingsWindow::setCurrentFolder(const QString& folder)
{
    m_currentFolder = folder;
    updateStorageLabel();
}

void SettingsWindow::updateStorageLabel()
{
    // Falls back to the drive this app is installed on when no library
    // folder is set yet, so the readout is never just blank.
    const QString path = m_currentFolder.isEmpty() ? QCoreApplication::applicationDirPath() : m_currentFolder;
    const QStorageInfo storage(path);
    if (!storage.isValid()) {
        m_storageLabel->clear();
        return;
    }

    const QLocale locale;
    m_storageLabel->setText(QStringLiteral("%1 free of %2")
                                 .arg(locale.formattedDataSize(storage.bytesAvailable()),
                                      locale.formattedDataSize(storage.bytesTotal())));
}

void SettingsWindow::setMediaItem(const MediaItem& item)
{
    m_currentFolder = item.libraryFolder;
    updateStorageLabel();
    m_closeToTray = item.closeToTray;
    m_muteCheckBox->setChecked(item.muted);
    m_volumeSlider->setValue(item.volume);
    m_volumeSlider->setEnabled(!item.muted);

    const int index = m_fitModeCombo->findData(static_cast<int>(item.fitMode));
    if (index >= 0)
        m_fitModeCombo->setCurrentIndex(index);

    m_zoomSlider->setValue(static_cast<int>(item.zoom * 100));
    m_flipHCheckBox->setChecked(item.flipHorizontal);
    m_flipVCheckBox->setChecked(item.flipVertical);
    m_showDesktopIconsCheckBox->setChecked(item.showDesktopIcons);
    m_brightnessSlider->setValue(item.brightness);
    m_contrastSlider->setValue(item.contrast);
    m_saturationSlider->setValue(item.saturation);
    m_speedSlider->setValue(static_cast<int>(item.playbackRate * 100));
    const int frameRateIndex = m_frameRateLimitCombo->findData(item.previewFrameRateLimit);
    m_frameRateLimitCombo->setCurrentIndex(frameRateIndex >= 0 ? frameRateIndex : 0);
    m_animatedBackground->setTheme(item.backgroundTheme);

    if (!item.filePath.isEmpty()) {
        m_previewedPath = item.filePath;
        m_previewFilenameLabel->setText(QFileInfo(item.filePath).fileName());
        updateFileInfo(item.filePath);
    }
}

void SettingsWindow::setLibraryFiles(const QStringList& filePaths, const QString& selectedPath)
{
    m_libraryGrid->clear();

    const QIcon placeholderIcon = style()->standardIcon(QStyle::SP_MediaPlay);

    for (const QString& path : filePaths) {
        const QFileInfo info(path);
        auto* item = new QListWidgetItem(info.fileName());
        item->setData(Qt::UserRole, path);
        item->setTextAlignment(Qt::AlignHCenter);
        item->setIcon(placeholderIcon);

        if (LibraryScanner::isGif(path)) {
            QImageReader reader(path);
            const int frameCount = reader.imageCount();
            QList<QPixmap> frames;
            if (frameCount > 1) {
                const int step = qMax(1, frameCount / 6);
                for (int i = 0; i < frameCount; i += step) {
                    reader.jumpToImage(i);
                    const QImage frame = reader.read();
                    if (!frame.isNull())
                        frames.append(scaledThumbnail(QPixmap::fromImage(frame)));
                }
            } else {
                QImageReader singleReader(path);
                const QImage frame = singleReader.read();
                if (!frame.isNull())
                    frames.append(scaledThumbnail(QPixmap::fromImage(frame)));
            }
            if (!frames.isEmpty())
                item->setData(LibraryItemDelegate::FramesRole, QVariant::fromValue(frames));
        }

        m_libraryGrid->addItem(item);
    }

    if (QListWidgetItem* selected = findItem(selectedPath))
        m_libraryGrid->setCurrentItem(selected);

    m_countLabel->setText(m_currentFolder.isEmpty()
                               ? QStringLiteral("No folder selected")
                               : QStringLiteral("%1  —  %2 wallpaper%3")
                                     .arg(m_currentFolder)
                                     .arg(filePaths.size())
                                     .arg(filePaths.size() == 1 ? QString() : QStringLiteral("s")));
}

void SettingsWindow::setFrames(const QString& filePath, const QList<QPixmap>& frames)
{
    if (frames.isEmpty())
        return;

    if (QListWidgetItem* item = findItem(filePath))
        item->setData(LibraryItemDelegate::FramesRole, QVariant::fromValue(frames));
}

void* SettingsWindow::previewNativeHandle() const
{
    return reinterpret_cast<void*>(m_previewWidget->winId());
}

void SettingsWindow::setVideoResolution(int width, int height)
{
    m_fileResolutionLabel->setText(QStringLiteral("%1 × %2").arg(width).arg(height));
}

void SettingsWindow::updateFileInfo(const QString& path)
{
    const QFileInfo info(path);
    m_fileSizeLabel->setText(humanFileSize(info.size()));
    m_fileResolutionLabel->setText(QStringLiteral("—"));
}

void SettingsWindow::advanceFrame()
{
    ++m_currentFrame;
    m_delegate->setCurrentFrame(m_currentFrame);
    if (m_libraryGrid->viewport())
        m_libraryGrid->viewport()->update();
}

void SettingsWindow::closeEvent(QCloseEvent* event)
{
    chromaDebugLog(QStringLiteral("SettingsWindow::closeEvent closeToTray=%1").arg(m_closeToTray));
    if (m_closeToTray) {
        event->ignore();
        hide();
        chromaDebugLog(QStringLiteral("SettingsWindow::closeEvent hide() returned"));
    } else {
        event->accept();
    }
}

void SettingsWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && m_titleBar)
        m_titleBar->setMaximized(isMaximized());
}

void SettingsWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (m_animatedBackground)
        m_animatedBackground->setGeometry(rect());
}

#ifdef _WIN32
bool SettingsWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    // Drag-to-move is handled separately, at the widget level, via
    // CustomTitleBar::mousePressEvent + QWindow::startSystemMove() - see
    // that class's header comment for why returning HTCAPTION from here
    // wasn't reliable for actual mouse-driven dragging. This function now
    // only handles resize-from-edges.
    auto* msg = static_cast<MSG*>(message);
    if (msg->message == WM_NCHITTEST && !isMaximized()) {
        constexpr int kBorderLogical = 6;

        // Native/physical pixels throughout - the same space WM_NCHITTEST's
        // lParam and GetWindowRect already use - rather than converting
        // through Qt's logical/DPI-scaled widget coordinates.
        const double dpr = devicePixelRatioF();
        RECT windowRect;
        GetWindowRect(reinterpret_cast<HWND>(winId()), &windowRect);

        const int x = GET_X_LPARAM(msg->lParam);
        const int y = GET_Y_LPARAM(msg->lParam);
        const int borderPhysical = qRound(kBorderLogical * dpr);

        const bool left = x < windowRect.left + borderPhysical;
        const bool right = x >= windowRect.right - borderPhysical;
        const bool top = y < windowRect.top + borderPhysical;
        const bool bottom = y >= windowRect.bottom - borderPhysical;

        if (top && left) { *result = HTTOPLEFT; return true; }
        if (top && right) { *result = HTTOPRIGHT; return true; }
        if (bottom && left) { *result = HTBOTTOMLEFT; return true; }
        if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
        if (left) { *result = HTLEFT; return true; }
        if (right) { *result = HTRIGHT; return true; }
        if (top) { *result = HTTOP; return true; }
        if (bottom) { *result = HTBOTTOM; return true; }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

QListWidgetItem* SettingsWindow::findItem(const QString& filePath) const
{
    if (filePath.isEmpty())
        return nullptr;

    for (int i = 0; i < m_libraryGrid->count(); ++i) {
        QListWidgetItem* item = m_libraryGrid->item(i);
        if (item->data(Qt::UserRole).toString() == filePath)
            return item;
    }
    return nullptr;
}

} // namespace colorfy
