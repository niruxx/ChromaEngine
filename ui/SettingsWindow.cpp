#include "SettingsWindow.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QListWidget>
#include <QSlider>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

#include "LibraryItemDelegate.h"
#include "colorfy/LibraryScanner.h"

namespace colorfy {

namespace {

constexpr int kThumbnailWidth = 160;
constexpr int kThumbnailHeight = 90;

QPixmap scaledThumbnail(const QPixmap& source)
{
    return source.scaled(kThumbnailWidth, kThumbnailHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
}

} // namespace

SettingsWindow::SettingsWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Colorfy Engine"));
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget#libraryPanel, QWidget#previewPanel { background-color: #17171a; }
        QWidget { color: #e6e6e6; }
        QToolBar {
            background-color: #1c1c20;
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
            background-color: #1c1c20;
            border-top: 1px solid #2a2a2e;
            color: #9a9aa0;
        }
        QLabel#sectionTitle {
            color: #8a8a92;
            font-size: 11px;
            font-weight: 600;
            letter-spacing: 1px;
            padding-bottom: 4px;
        }
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
        QCheckBox::indicator {
            width: 16px; height: 16px;
            border-radius: 4px;
            border: 1px solid #4a4a52;
            background-color: #222226;
        }
        QCheckBox::indicator:checked {
            background-color: #5ba8e6;
            border-color: #5ba8e6;
        }
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
        QComboBox {
            background-color: #222226;
            border: 1px solid #3a3a40;
            border-radius: 5px;
            padding: 4px 8px;
        }
        QSplitter::handle {
            background-color: #17171a;
            width: 1px;
        }
    )"));

    buildUi();
    resize(980, 620);
}

void SettingsWindow::buildUi()
{
    auto* toolbar = addToolBar(QStringLiteral("Main"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setIconSize(QSize(18, 18));

    auto* openFolderAction = toolbar->addAction(style()->standardIcon(QStyle::SP_DirOpenIcon),
                                                 QStringLiteral("Open Folder"));
    connect(openFolderAction, &QAction::triggered, this, [this] {
        const QString folder = QFileDialog::getExistingDirectory(
            this, QStringLiteral("Choose wallpaper folder"), m_currentFolder);
        if (!folder.isEmpty())
            emit folderChanged(folder);
    });

    auto* refreshAction = toolbar->addAction(style()->standardIcon(QStyle::SP_BrowserReload),
                                              QStringLiteral("Refresh"));
    connect(refreshAction, &QAction::triggered, this, &SettingsWindow::refreshRequested);

    toolbar->addSeparator();

    m_playPauseAction = toolbar->addAction(style()->standardIcon(QStyle::SP_MediaPause), QStringLiteral("Pause"));
    m_playPauseAction->setCheckable(true);
    connect(m_playPauseAction, &QAction::toggled, this, [this](bool paused) {
        m_playPauseAction->setText(paused ? QStringLiteral("Play") : QStringLiteral("Pause"));
        m_playPauseAction->setIcon(style()->standardIcon(paused ? QStyle::SP_MediaPlay : QStyle::SP_MediaPause));
        emit playPauseToggled(paused);
    });

    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

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
    m_libraryGrid->setItemDelegate(new LibraryItemDelegate(m_libraryGrid));
    m_libraryGrid->setFrameShape(QFrame::NoFrame);
    connect(m_libraryGrid, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        const QString path = item->data(Qt::UserRole).toString();
        m_previewFilenameLabel->setText(QFileInfo(path).fileName());
        emit mediaSelected(path);
    });
    libraryLayout->addWidget(m_libraryGrid, 1);

    splitter->addWidget(libraryPanel);

    // --- Preview panel -------------------------------------------------
    auto* previewPanel = new QWidget(this);
    previewPanel->setObjectName(QStringLiteral("previewPanel"));
    previewPanel->setMinimumWidth(300);
    previewPanel->setMaximumWidth(420);
    auto* previewLayout = new QVBoxLayout(previewPanel);
    previewLayout->setContentsMargins(6, 12, 14, 12);

    auto* previewTitle = new QLabel(QStringLiteral("PREVIEW"), this);
    previewTitle->setObjectName(QStringLiteral("sectionTitle"));
    previewLayout->addWidget(previewTitle);

    auto* previewFrame = new QFrame(this);
    previewFrame->setObjectName(QStringLiteral("previewFrame"));
    previewFrame->setFixedHeight(220);
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

    previewLayout->addSpacing(10);

    auto* muteRow = new QHBoxLayout();
    m_muteCheckBox = new QCheckBox(QStringLiteral("Mute"), this);
    m_muteCheckBox->setChecked(true);
    connect(m_muteCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_volumeSlider->setEnabled(!checked);
        emit mutedChanged(checked);
    });
    muteRow->addWidget(m_muteCheckBox);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setEnabled(false);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &SettingsWindow::volumeChanged);
    muteRow->addWidget(m_volumeSlider, 1);
    previewLayout->addLayout(muteRow);

    auto* fitRow = new QHBoxLayout();
    auto* fitLabel = new QLabel(QStringLiteral("Fit mode"), this);
    fitRow->addWidget(fitLabel);
    m_fitModeCombo = new QComboBox(this);
    m_fitModeCombo->addItem(QStringLiteral("Fill"), static_cast<int>(FitMode::Fill));
    m_fitModeCombo->addItem(QStringLiteral("Fit"), static_cast<int>(FitMode::Fit));
    m_fitModeCombo->addItem(QStringLiteral("Stretch"), static_cast<int>(FitMode::Stretch));
    m_fitModeCombo->addItem(QStringLiteral("Center"), static_cast<int>(FitMode::Center));
    connect(m_fitModeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        emit fitModeChanged(static_cast<FitMode>(m_fitModeCombo->itemData(index).toInt()));
    });
    fitRow->addWidget(m_fitModeCombo, 1);
    previewLayout->addLayout(fitRow);

    previewLayout->addStretch(1);

    splitter->addWidget(previewPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);

    setCentralWidget(splitter);

    m_countLabel = new QLabel(QStringLiteral("No folder selected"), this);
    statusBar()->addWidget(m_countLabel);
}

void SettingsWindow::setMediaItem(const MediaItem& item)
{
    m_currentFolder = item.libraryFolder;
    m_muteCheckBox->setChecked(item.muted);
    m_volumeSlider->setValue(item.volume);
    m_volumeSlider->setEnabled(!item.muted);
    const int index = m_fitModeCombo->findData(static_cast<int>(item.fitMode));
    if (index >= 0)
        m_fitModeCombo->setCurrentIndex(index);

    if (!item.filePath.isEmpty())
        m_previewFilenameLabel->setText(QFileInfo(item.filePath).fileName());
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

        if (LibraryScanner::isGif(path)) {
            QImageReader reader(path);
            const QImage frame = reader.read();
            item->setIcon(!frame.isNull() ? QIcon(scaledThumbnail(QPixmap::fromImage(frame))) : placeholderIcon);
        } else {
            item->setIcon(placeholderIcon);
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

void SettingsWindow::setThumbnail(const QString& filePath, const QPixmap& pixmap)
{
    if (pixmap.isNull())
        return;

    if (QListWidgetItem* item = findItem(filePath))
        item->setIcon(QIcon(scaledThumbnail(pixmap)));
}

void* SettingsWindow::previewNativeHandle() const
{
    return reinterpret_cast<void*>(m_previewWidget->winId());
}

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
