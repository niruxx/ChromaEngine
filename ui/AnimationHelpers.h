#pragma once

#include <QAbstractButton>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QWidget>

namespace colorfy::AnimationHelpers {

// Brief opacity "tap" feedback on press/release. Deliberately animates
// opacity rather than geometry - a layout-managed widget's geometry is
// owned by its layout and fighting that produces jitter, while opacity is
// purely a paint-time effect and plays nicely with any layout.
inline void installPressAnimation(QAbstractButton* button)
{
    auto* effect = new QGraphicsOpacityEffect(button);
    effect->setOpacity(1.0);
    button->setGraphicsEffect(effect);

    QObject::connect(button, &QAbstractButton::pressed, button, [button, effect] {
        auto* anim = new QPropertyAnimation(effect, "opacity", button);
        anim->setDuration(70);
        anim->setStartValue(effect->opacity());
        anim->setEndValue(0.55);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });
    QObject::connect(button, &QAbstractButton::released, button, [button, effect] {
        auto* anim = new QPropertyAnimation(effect, "opacity", button);
        anim->setDuration(150);
        anim->setStartValue(effect->opacity());
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

// Fades a top-level widget in when shown.
inline void fadeIn(QWidget* widget, int durationMs = 220)
{
    auto* effect = new QGraphicsOpacityEffect(widget);
    widget->setGraphicsEffect(effect);
    auto* anim = new QPropertyAnimation(effect, "opacity", widget);
    anim->setDuration(durationMs);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(anim, &QPropertyAnimation::finished, widget, [widget] { widget->setGraphicsEffect(nullptr); });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace colorfy::AnimationHelpers
