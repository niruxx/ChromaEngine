#include "IconFactory.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

#include <cmath>
#include <functional>

namespace colorfy::IconFactory {

namespace {

constexpr double kPi = 3.14159265358979323846;

const QColor kStroke(0xc8, 0xc8, 0xce);
const QColor kAccent(0x5b, 0xa8, 0xe6);

QIcon build(int size, const std::function<void(QPainter&, double)>& draw)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    draw(painter, size);
    painter.end();

    return QIcon(pixmap);
}

QPen strokePen(double s, const QColor& color = kStroke)
{
    QPen pen(color);
    pen.setWidthF(s * 0.08);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

} // namespace

QIcon folder(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(strokePen(s));
        p.setBrush(Qt::NoBrush);

        QPainterPath path;
        path.moveTo(s * 0.12, s * 0.28);
        path.lineTo(s * 0.38, s * 0.28);
        path.lineTo(s * 0.46, s * 0.38);
        path.lineTo(s * 0.88, s * 0.38);
        path.lineTo(s * 0.88, s * 0.8);
        path.lineTo(s * 0.12, s * 0.8);
        path.closeSubpath();
        p.drawPath(path);
    });
}

QIcon refresh(int size)
{
    return build(size, [](QPainter& p, double s) {
        QPen pen = strokePen(s);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        QRectF rect(s * 0.2, s * 0.2, s * 0.6, s * 0.6);
        p.drawArc(rect, 30 * 16, 300 * 16);

        // arrowhead at the open end of the arc
        const double angle = 330.0 * kPi / 180.0;
        const QPointF tip(rect.center().x() + (rect.width() / 2) * std::cos(angle),
                           rect.center().y() - (rect.height() / 2) * std::sin(angle));
        QPainterPath arrow;
        arrow.moveTo(tip + QPointF(-s * 0.1, -s * 0.02));
        arrow.lineTo(tip + QPointF(s * 0.03, s * 0.12));
        arrow.lineTo(tip + QPointF(s * 0.13, -s * 0.02));
        p.setBrush(pen.color());
        p.setPen(Qt::NoPen);
        p.drawPolygon(arrow.toFillPolygon());
    });
}

QIcon play(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(Qt::NoPen);
        p.setBrush(kAccent);
        QPolygonF triangle;
        triangle << QPointF(s * 0.3, s * 0.22) << QPointF(s * 0.3, s * 0.78) << QPointF(s * 0.82, s * 0.5);
        p.drawPolygon(triangle);
    });
}

QIcon pause(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(Qt::NoPen);
        p.setBrush(kAccent);
        p.drawRoundedRect(QRectF(s * 0.28, s * 0.22, s * 0.16, s * 0.56), s * 0.03, s * 0.03);
        p.drawRoundedRect(QRectF(s * 0.56, s * 0.22, s * 0.16, s * 0.56), s * 0.03, s * 0.03);
    });
}

QIcon gear(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(Qt::NoPen);
        p.setBrush(kStroke);

        const QPointF center(s * 0.5, s * 0.5);
        const double outerR = s * 0.38;
        const double innerR = s * 0.27;
        const int teeth = 8;

        QPainterPath path;
        for (int i = 0; i < teeth * 2; ++i) {
            const double angle = (kPi * i) / teeth;
            const double r = (i % 2 == 0) ? outerR : innerR;
            const QPointF pt(center.x() + r * std::cos(angle), center.y() + r * std::sin(angle));
            if (i == 0)
                path.moveTo(pt);
            else
                path.lineTo(pt);
        }
        path.closeSubpath();
        p.drawPath(path);

        p.setBrush(Qt::black);
        p.setCompositionMode(QPainter::CompositionMode_Clear);
        p.drawEllipse(center, s * 0.14, s * 0.14);
    });
}

QIcon monitor(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(strokePen(s));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(s * 0.12, s * 0.18, s * 0.76, s * 0.5), s * 0.04, s * 0.04);
        p.drawLine(QPointF(s * 0.5, s * 0.68), QPointF(s * 0.5, s * 0.8));
        p.drawLine(QPointF(s * 0.34, s * 0.82), QPointF(s * 0.66, s * 0.82));
    });
}

QIcon rename(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(strokePen(s));
        p.setBrush(Qt::NoBrush);

        p.save();
        p.translate(s * 0.5, s * 0.5);
        p.rotate(45);
        p.drawRoundedRect(QRectF(-s * 0.08, -s * 0.32, s * 0.16, s * 0.5), s * 0.03, s * 0.03);
        p.drawLine(QPointF(-s * 0.08, s * 0.1), QPointF(s * 0.08, s * 0.1));
        p.restore();
    });
}

QIcon trash(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(strokePen(s));
        p.setBrush(Qt::NoBrush);
        p.drawLine(QPointF(s * 0.22, s * 0.3), QPointF(s * 0.78, s * 0.3));
        p.drawLine(QPointF(s * 0.4, s * 0.3), QPointF(s * 0.42, s * 0.2));
        p.drawLine(QPointF(s * 0.42, s * 0.2), QPointF(s * 0.58, s * 0.2));
        p.drawLine(QPointF(s * 0.58, s * 0.2), QPointF(s * 0.6, s * 0.3));

        QPainterPath body;
        body.moveTo(s * 0.28, s * 0.3);
        body.lineTo(s * 0.32, s * 0.82);
        body.lineTo(s * 0.68, s * 0.82);
        body.lineTo(s * 0.72, s * 0.3);
        p.drawPath(body);

        p.drawLine(QPointF(s * 0.42, s * 0.42), QPointF(s * 0.44, s * 0.72));
        p.drawLine(QPointF(s * 0.58, s * 0.42), QPointF(s * 0.56, s * 0.72));
    });
}

QIcon applyCheck(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(Qt::NoPen);
        p.setBrush(kAccent);
        p.drawEllipse(QRectF(s * 0.14, s * 0.14, s * 0.72, s * 0.72));

        QPen check(Qt::white);
        check.setWidthF(s * 0.09);
        check.setCapStyle(Qt::RoundCap);
        check.setJoinStyle(Qt::RoundJoin);
        p.setPen(check);
        QPainterPath path;
        path.moveTo(s * 0.32, s * 0.5);
        path.lineTo(s * 0.45, s * 0.64);
        path.lineTo(s * 0.7, s * 0.36);
        p.drawPath(path);
    });
}

QIcon flipHorizontal(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(strokePen(s));
        QPen dashed = strokePen(s);
        dashed.setStyle(Qt::DashLine);
        p.setPen(dashed);
        p.drawLine(QPointF(s * 0.5, s * 0.15), QPointF(s * 0.5, s * 0.85));

        p.setPen(Qt::NoPen);
        p.setBrush(kStroke);
        QPolygonF left;
        left << QPointF(s * 0.4, s * 0.3) << QPointF(s * 0.4, s * 0.7) << QPointF(s * 0.2, s * 0.5);
        p.drawPolygon(left);
        QPolygonF right;
        right << QPointF(s * 0.6, s * 0.3) << QPointF(s * 0.6, s * 0.7) << QPointF(s * 0.8, s * 0.5);
        p.drawPolygon(right);
    });
}

QIcon flipVertical(int size)
{
    return build(size, [](QPainter& p, double s) {
        QPen dashed = strokePen(s);
        dashed.setStyle(Qt::DashLine);
        p.setPen(dashed);
        p.drawLine(QPointF(s * 0.15, s * 0.5), QPointF(s * 0.85, s * 0.5));

        p.setPen(Qt::NoPen);
        p.setBrush(kStroke);
        QPolygonF top;
        top << QPointF(s * 0.3, s * 0.4) << QPointF(s * 0.7, s * 0.4) << QPointF(s * 0.5, s * 0.2);
        p.drawPolygon(top);
        QPolygonF bottom;
        bottom << QPointF(s * 0.3, s * 0.6) << QPointF(s * 0.7, s * 0.6) << QPointF(s * 0.5, s * 0.8);
        p.drawPolygon(bottom);
    });
}

QIcon appLogo(int size)
{
    return build(size, [](QPainter& p, double s) {
        // "Chroma" gradient: pink -> violet -> cyan, matching resources/app_icon.ico.
        QLinearGradient gradient(0, 0, s, s);
        gradient.setColorAt(0.0, QColor(0xff, 0x4d, 0x6d));
        gradient.setColorAt(0.5, QColor(0x9b, 0x30, 0xff));
        gradient.setColorAt(1.0, QColor(0x00, 0xc9, 0xff));

        p.setPen(Qt::NoPen);
        p.setBrush(gradient);
        p.drawRoundedRect(QRectF(0, 0, s, s), s * 0.22, s * 0.22);

        p.setBrush(QColor(255, 255, 255, 235));
        QPolygonF triangle;
        triangle << QPointF(s * 0.4, s * 0.28) << QPointF(s * 0.4, s * 0.72) << QPointF(s * 0.76, s * 0.5);
        p.drawPolygon(triangle);
    });
}

QIcon windowMinimize(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(strokePen(s));
        p.drawLine(QPointF(s * 0.28, s * 0.5), QPointF(s * 0.72, s * 0.5));
    });
}

QIcon windowMaximize(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(strokePen(s));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(s * 0.28, s * 0.28, s * 0.44, s * 0.44));
    });
}

QIcon windowRestore(int size)
{
    return build(size, [](QPainter& p, double s) {
        QPen pen = strokePen(s);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(s * 0.34, s * 0.22, s * 0.38, s * 0.38));
        p.setBrush(QColor(0x17, 0x17, 0x1a));
        p.drawRect(QRectF(s * 0.22, s * 0.4, s * 0.38, s * 0.38));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(s * 0.22, s * 0.4, s * 0.38, s * 0.38));
    });
}

QIcon windowClose(int size)
{
    return build(size, [](QPainter& p, double s) {
        p.setPen(strokePen(s));
        p.drawLine(QPointF(s * 0.28, s * 0.28), QPointF(s * 0.72, s * 0.72));
        p.drawLine(QPointF(s * 0.72, s * 0.28), QPointF(s * 0.28, s * 0.72));
    });
}

} // namespace colorfy::IconFactory
