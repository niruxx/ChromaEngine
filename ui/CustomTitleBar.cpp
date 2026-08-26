#include "CustomTitleBar.h"

#include "IconFactory.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QToolButton>

namespace colorfy {

namespace {

QToolButton* makeButton(const QIcon& icon)
{
    auto* button = new QToolButton();
    button->setIcon(icon);
    button->setIconSize(QSize(14, 14));
    button->setFixedSize(CustomTitleBar::kButtonWidth, CustomTitleBar::kHeight);
    button->setCursor(Qt::ArrowCursor);
    return button;
}

} // namespace

CustomTitleBar::CustomTitleBar(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(kHeight);
    setStyleSheet(QStringLiteral(R"(
        CustomTitleBar { background-color: rgba(28, 28, 32, 190); }
        QLabel { color: #c8c8ce; font-weight: 500; background: transparent; }
        QToolButton { background: transparent; border: none; }
        QToolButton:hover { background-color: #2a2a30; }
        QToolButton#closeButton:hover { background-color: #c0392b; }
    )"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(0);

    auto* iconLabel = new QLabel(this);
    iconLabel->setPixmap(IconFactory::appLogo(16).pixmap(16, 16));
    // Purely decorative - without this, a press on the icon/title text would
    // be swallowed here (QLabel doesn't forward unhandled mouse events to
    // its parent) instead of reaching CustomTitleBar::mousePressEvent below,
    // making large, obvious-looking parts of the bar not actually draggable.
    iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addWidget(iconLabel);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setContentsMargins(8, 0, 0, 0);
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addWidget(titleLabel);

    layout->addStretch(1);

    m_minimizeButton = makeButton(IconFactory::windowMinimize());
    connect(m_minimizeButton, &QToolButton::clicked, this, &CustomTitleBar::minimizeRequested);
    layout->addWidget(m_minimizeButton);

    m_maximizeButton = makeButton(IconFactory::windowMaximize());
    connect(m_maximizeButton, &QToolButton::clicked, this, &CustomTitleBar::maximizeRestoreRequested);
    layout->addWidget(m_maximizeButton);

    m_closeButton = makeButton(IconFactory::windowClose());
    m_closeButton->setObjectName(QStringLiteral("closeButton"));
    connect(m_closeButton, &QToolButton::clicked, this, &CustomTitleBar::closeRequested);
    layout->addWidget(m_closeButton);
}

void CustomTitleBar::setMaximized(bool maximized)
{
    m_maximized = maximized;
    m_maximizeButton->setIcon(maximized ? IconFactory::windowRestore() : IconFactory::windowMaximize());
}

QWidget* CustomTitleBar::minimizeButton() const
{
    return m_minimizeButton;
}

QWidget* CustomTitleBar::maximizeButton() const
{
    return m_maximizeButton;
}

QWidget* CustomTitleBar::closeButton() const
{
    return m_closeButton;
}

bool CustomTitleBar::isOnButton(const QPoint& pos) const
{
    QWidget* child = childAt(pos);
    return child == m_minimizeButton || child == m_maximizeButton || child == m_closeButton;
}

void CustomTitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && !isOnButton(event->pos())) {
        m_dragging = true;
        m_dragStartGlobalPos = event->globalPosition().toPoint();
        m_dragStartWindowPos = window()->pos();
        emit dragStarted();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        const QPoint delta = event->globalPosition().toPoint() - m_dragStartGlobalPos;
        window()->move(m_dragStartWindowPos + delta);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void CustomTitleBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        emit dragFinished();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && !isOnButton(event->pos())) {
        emit maximizeRestoreRequested();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

} // namespace colorfy
