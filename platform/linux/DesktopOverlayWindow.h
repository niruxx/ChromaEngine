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
// the wallpaper. Kept as a normal WM-managed top-level window (unlike
// X11WallpaperWindow, which bypasses the window manager entirely) with
// Qt::WindowStaysOnBottomHint (X11: _NET_WM_STATE_BELOW) so it stays below
// normal application windows but above the desktop/wallpaper layer - actual
// alpha transparency here needs a compositor running, which is the default
// in XFCE (xfwm4's built-in compositor) and modern MATE (Marco), but can be
// turned off by the user, in which case this window falls back to opaque.
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
