#pragma once

#include <QHash>
#include <QRect>
#include <QWidget>

#include "BluetoothBatteryReader.h"
#include "colorfy/MediaItem.h"

class QTimer;
class QPainter;

namespace colorfy {

// A fully transparent, click-through window the same size as the monitor,
// drawing the optional clock/calendar/Bluetooth-battery widgets on top of
// the wallpaper.
//
// Unlike WallpaperWindow, this is deliberately NOT reparented into WorkerW:
// per-pixel-alpha ("layered") compositing was found to not actually render
// once a window is forced into WS_CHILD via SetParent in that hierarchy
// (confirmed live - the window existed, was visible, and had the correct
// WS_EX_LAYERED style, but nothing was ever visibly composited). Kept as a
// genuine top-level window with Qt::WindowStaysOnBottomHint instead, where
// layered transparency is unambiguously supported. Tradeoff: it can render
// above desktop icons in the small screen regions it actually draws into,
// rather than strictly behind them like the wallpaper itself.
class DesktopOverlayWindow : public QWidget {
    Q_OBJECT
public:
    explicit DesktopOverlayWindow(const QRect& geometry, QWidget* parent = nullptr);

    void* nativeHandle() const;
    void showOnDesktop();
    void applySettings(const MediaItem& media);

    bool anyWidgetEnabled() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void refreshBattery();

    QColor themeTextColor(OverlayTheme theme) const;
    QColor themeOutlineColor(OverlayTheme theme) const;
    void drawStyledText(QPainter& painter, const QRect& rect, Qt::Alignment alignment, const QFont& font,
                         const QString& text, OverlayTheme theme);
    QRect computeAnchorRect(OverlayPosition position, int margin, const QSize& size, QHash<int, int>& stackOffsets);

    void paintClock(QPainter& painter, QHash<int, int>& stackOffsets);
    void paintAnalogClock(QPainter& painter, const QRect& rect, const QTime& time, OverlayTheme theme);
    void paintCalendar(QPainter& painter, QHash<int, int>& stackOffsets);
    void paintBattery(QPainter& painter, QHash<int, int>& stackOffsets);

    QRect m_geometry;
    MediaItem m_settings;
    QTimer* m_tickTimer = nullptr;
    QTimer* m_batteryTimer = nullptr;
    QList<BluetoothDeviceBattery> m_batteryDevices;
};

} // namespace colorfy
