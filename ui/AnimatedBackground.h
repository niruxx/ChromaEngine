#pragma once

#include <QWidget>

class QTimer;

namespace colorfy {

// Paints an optional slow, subtle animated background for the library
// panel. Purely decorative - sits behind the (transparent-background)
// library grid. Theme::None matches the app's plain flat panel color.
class AnimatedBackground : public QWidget {
    Q_OBJECT
public:
    enum Theme { None = 0, Aurora = 1, Starfield = 2 };

    explicit AnimatedBackground(QWidget* parent = nullptr);

    void setTheme(int theme);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void paintAurora(class QPainter& painter);
    void paintStarfield(class QPainter& painter);

    Theme m_theme = None;
    QTimer* m_timer = nullptr;
    double m_phase = 0.0;

    struct Star {
        QPointF pos;
        double phase;
        double speed;
        double radius;
    };
    QList<Star> m_stars;
    void ensureStars();
};

} // namespace colorfy
