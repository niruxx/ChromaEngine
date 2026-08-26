#include "AnimatedBackground.h"

#include <QPainter>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <QTimer>

#include <cmath>

namespace colorfy {

namespace {
constexpr int kTimerIntervalMs = 60;
constexpr int kStarCount = 70;
constexpr double kTwoPi = 6.28318530717958647692;
} // namespace

AnimatedBackground::AnimatedBackground(QWidget* parent)
    : QWidget(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this] {
        m_phase += kTimerIntervalMs / 1000.0;
        update();
    });
}

void AnimatedBackground::setTheme(int theme)
{
    m_theme = static_cast<Theme>(theme);
    if (m_theme != None && !m_timer->isActive())
        m_timer->start(kTimerIntervalMs);
    else if (m_theme == None)
        m_timer->stop();
    update();
}

void AnimatedBackground::ensureStars()
{
    if (!m_stars.isEmpty() || width() <= 0 || height() <= 0)
        return;

    QRandomGenerator* rng = QRandomGenerator::global();
    m_stars.reserve(kStarCount);
    for (int i = 0; i < kStarCount; ++i) {
        Star star;
        star.pos = QPointF(rng->bounded(width()), rng->bounded(height()));
        star.phase = rng->bounded(1000) / 1000.0 * kTwoPi;
        star.speed = 0.6 + rng->bounded(1000) / 1000.0 * 1.2;
        star.radius = 0.8 + rng->bounded(1000) / 1000.0 * 1.6;
        m_stars.append(star);
    }
}

void AnimatedBackground::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(0x17, 0x17, 0x1a));

    switch (m_theme) {
    case Aurora:
        paintAurora(painter);
        break;
    case Starfield:
        paintStarfield(painter);
        break;
    case None:
    default:
        break;
    }
}

void AnimatedBackground::paintAurora(QPainter& painter)
{
    const double w = width();
    const double h = height();
    if (w <= 0 || h <= 0)
        return;

    struct Blob {
        QColor color;
        double speedX, speedY, radius, phaseOffset;
    };
    static const Blob blobs[] = {
        {QColor(0x6a, 0x5a, 0xe0, 60), 0.11, 0.14, 0.55, 0.0},
        {QColor(0x2f, 0x8f, 0xd6, 55), 0.09, 0.17, 0.5, 2.1},
        {QColor(0x5b, 0xa8, 0xe6, 45), 0.13, 0.10, 0.45, 4.2},
    };

    painter.setPen(Qt::NoPen);
    for (const Blob& blob : blobs) {
        const double cx = w * (0.5 + 0.35 * std::sin(m_phase * blob.speedX + blob.phaseOffset));
        const double cy = h * (0.5 + 0.35 * std::cos(m_phase * blob.speedY + blob.phaseOffset));
        const double radius = std::min(w, h) * blob.radius;

        QRadialGradient gradient(cx, cy, radius);
        gradient.setColorAt(0.0, blob.color);
        gradient.setColorAt(1.0, QColor(blob.color.red(), blob.color.green(), blob.color.blue(), 0));
        painter.setBrush(gradient);
        painter.drawEllipse(QPointF(cx, cy), radius, radius);
    }
}

void AnimatedBackground::paintStarfield(QPainter& painter)
{
    ensureStars();

    painter.setPen(Qt::NoPen);
    for (const Star& star : m_stars) {
        const double twinkle = 0.4 + 0.6 * (0.5 + 0.5 * std::sin(m_phase * star.speed + star.phase));
        painter.setBrush(QColor(255, 255, 255, static_cast<int>(twinkle * 160)));
        painter.drawEllipse(star.pos, star.radius, star.radius);
    }
}

} // namespace colorfy
