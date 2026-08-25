#pragma once

#include <QStyledItemDelegate>

namespace colorfy {

// Draws library tiles as rounded preview cards with a gradient-shaded
// filename caption and an accent-colored selection outline, instead of
// QListWidget's default flat icon+label rendering.
class LibraryItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit LibraryItemDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

} // namespace colorfy
