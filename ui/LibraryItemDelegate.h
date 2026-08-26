#pragma once

#include <QList>
#include <QPixmap>
#include <QStyledItemDelegate>

Q_DECLARE_METATYPE(QList<QPixmap>)

namespace colorfy {

// Draws library tiles as rounded preview cards with a gradient-shaded
// filename caption and an accent-colored selection outline, instead of
// QListWidget's default flat icon+label rendering. Each item can carry a
// short sequence of frames (FramesRole); setCurrentFrame() (driven by a
// shared timer in SettingsWindow) cycles every visible tile through its
// sequence in lock-step, giving the grid an "auto-playing" look without a
// native window per tile.
class LibraryItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    static constexpr int FramesRole = Qt::UserRole + 10;

    explicit LibraryItemDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

public slots:
    void setCurrentFrame(int frame);

private:
    int m_currentFrame = 0;
};

} // namespace colorfy
