#include "LibraryItemDelegate.h"

#include <QIcon>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

namespace colorfy {

namespace {
constexpr int kTileWidth = 176;
constexpr int kTileHeight = 128;
constexpr int kCornerRadius = 10;
constexpr int kCaptionHeight = 34;
} // namespace

LibraryItemDelegate::LibraryItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void LibraryItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QRect tileRect = option.rect.adjusted(4, 4, -4, -4);
    QPainterPath cardPath;
    cardPath.addRoundedRect(tileRect, kCornerRadius, kCornerRadius);
    painter->setClipPath(cardPath);

    painter->fillPath(cardPath, QColor(0x18, 0x18, 0x1a));

    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        const QPixmap pixmap = icon.pixmap(tileRect.size());
        const QPixmap scaled = pixmap.scaled(tileRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const QPoint center(tileRect.center().x() - scaled.width() / 2, tileRect.center().y() - scaled.height() / 2);
        painter->drawPixmap(center, scaled);
    }

    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;

    if (hovered && !selected)
        painter->fillPath(cardPath, QColor(255, 255, 255, 18));

    QRect captionRect = tileRect;
    captionRect.setTop(tileRect.bottom() - kCaptionHeight);
    QLinearGradient gradient(captionRect.topLeft(), captionRect.bottomLeft());
    gradient.setColorAt(0.0, QColor(0, 0, 0, 0));
    gradient.setColorAt(1.0, QColor(0, 0, 0, 200));
    painter->fillRect(captionRect, gradient);

    painter->setPen(Qt::white);
    const QString text = index.data(Qt::DisplayRole).toString();
    const QRect textRect = captionRect.adjusted(8, 0, -8, -6);
    const QString elided = option.fontMetrics.elidedText(text, Qt::ElideMiddle, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignBottom, elided);

    painter->setClipping(false);
    if (selected) {
        QPen pen(QColor(0x5b, 0xa8, 0xe6), 2.5);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(cardPath);
    } else {
        painter->setPen(QColor(255, 255, 255, 20));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(cardPath);
    }

    painter->restore();
}

QSize LibraryItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    return QSize(kTileWidth, kTileHeight);
}

} // namespace colorfy
