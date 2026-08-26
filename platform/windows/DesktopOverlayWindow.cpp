#include "DesktopOverlayWindow.h"

#include <QDate>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QTime>
#include <QTimer>

#include <windows.h>

#include <cmath>

namespace colorfy {

namespace {
constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
constexpr int kTickIntervalMs = 1000;
constexpr int kBatteryRefreshMs = 60000;
} // namespace

DesktopOverlayWindow::DesktopOverlayWindow(const QRect& geometry, QWidget* parent)
    : QWidget(parent)
    , m_geometry(geometry)
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_TranslucentBackground);
    // Genuine top-level window (see the class comment for why this isn't
    // reparented into WorkerW like WallpaperWindow): stays below normal
    // application windows but above the desktop/wallpaper layer.
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnBottomHint | Qt::WindowDoesNotAcceptFocus);
    setGeometry(m_geometry);

    winId();

    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);

    m_tickTimer = new QTimer(this);
    connect(m_tickTimer, &QTimer::timeout, this, [this] { update(); });
    m_tickTimer->start(kTickIntervalMs);

    m_batteryTimer = new QTimer(this);
    connect(m_batteryTimer, &QTimer::timeout, this, &DesktopOverlayWindow::refreshBattery);
}

void* DesktopOverlayWindow::nativeHandle() const
{
    return reinterpret_cast<void*>(winId());
}

bool DesktopOverlayWindow::anyWidgetEnabled() const
{
    return m_settings.clockEnabled || m_settings.calendarEnabled || m_settings.batteryIndicatorEnabled;
}

void DesktopOverlayWindow::showOnDesktop()
{
    setGeometry(m_geometry);
    show();
    HWND hwnd = reinterpret_cast<HWND>(winId());
    SetWindowPos(hwnd, HWND_BOTTOM, m_geometry.x(), m_geometry.y(), m_geometry.width(), m_geometry.height(),
                 SWP_NOACTIVATE);
}

void DesktopOverlayWindow::applySettings(const MediaItem& media)
{
    const bool batteryWasEnabled = m_settings.batteryIndicatorEnabled;
    m_settings = media;

    if (m_settings.batteryIndicatorEnabled) {
        if (!batteryWasEnabled || !m_batteryTimer->isActive()) {
            refreshBattery();
            m_batteryTimer->start(kBatteryRefreshMs);
        }
    } else {
        m_batteryTimer->stop();
    }

    update();
}

void DesktopOverlayWindow::refreshBattery()
{
    m_batteryDevices = BluetoothBatteryReader::queryConnectedDevices();
    update();
}

QColor DesktopOverlayWindow::themeTextColor(OverlayTheme theme) const
{
    switch (theme) {
    case OverlayTheme::Light:
        return QColor(255, 255, 255);
    case OverlayTheme::Dark:
        return QColor(25, 25, 25);
    case OverlayTheme::Accent:
        return QColor(0x5b, 0xa8, 0xe6);
    case OverlayTheme::Outline:
        return QColor(255, 255, 255, 0);
    }
    return Qt::white;
}

QColor DesktopOverlayWindow::themeOutlineColor(OverlayTheme theme) const
{
    switch (theme) {
    case OverlayTheme::Light:
        return QColor(0, 0, 0, 190);
    case OverlayTheme::Dark:
        return QColor(255, 255, 255, 210);
    case OverlayTheme::Accent:
        return QColor(0, 0, 0, 190);
    case OverlayTheme::Outline:
        return QColor(255, 255, 255, 235);
    }
    return QColor(0, 0, 0, 190);
}

void DesktopOverlayWindow::drawStyledText(QPainter& painter, const QRect& rect, Qt::Alignment alignment,
                                           const QFont& font, const QString& text, OverlayTheme theme)
{
    painter.setFont(font);

    static const QPoint kOutlineOffsets[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}, {0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    painter.setPen(themeOutlineColor(theme));
    for (const QPoint& offset : kOutlineOffsets)
        painter.drawText(rect.translated(offset), alignment, text);

    painter.setPen(themeTextColor(theme));
    painter.drawText(rect, alignment, text);
}

QRect DesktopOverlayWindow::computeAnchorRect(OverlayPosition position, int margin, const QSize& size,
                                               QHash<int, int>& stackOffsets)
{
    const int extra = stackOffsets.value(static_cast<int>(position), 0);
    int x = margin;
    int y = margin;

    // Anchor math must use this widget's own logical (device-independent)
    // size, not m_geometry's native-pixel monitor size: QPainter draws in
    // logical coordinates, which differ from native pixels whenever the
    // monitor's DPI scale isn't 100% (confirmed live - anchors computed
    // from m_geometry landed outside the widget's actual paintable area
    // and were silently clipped, while TopLeft happened to still fall
    // inside both coordinate spaces and so appeared to work).
    const int logicalWidth = width();
    const int logicalHeight = height();

    switch (position) {
    case OverlayPosition::TopLeft:
        x = margin;
        y = margin + extra;
        break;
    case OverlayPosition::TopRight:
        x = logicalWidth - margin - size.width();
        y = margin + extra;
        break;
    case OverlayPosition::BottomLeft:
        x = margin;
        y = logicalHeight - margin - size.height() - extra;
        break;
    case OverlayPosition::BottomRight:
        x = logicalWidth - margin - size.width();
        y = logicalHeight - margin - size.height() - extra;
        break;
    case OverlayPosition::Center:
        x = (logicalWidth - size.width()) / 2;
        y = (logicalHeight - size.height()) / 2 + extra;
        break;
    }

    stackOffsets[static_cast<int>(position)] = extra + size.height() + 14;
    return QRect(QPoint(x, y), size);
}

void DesktopOverlayWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    QHash<int, int> stackOffsets;
    paintClock(painter, stackOffsets);
    paintCalendar(painter, stackOffsets);
    paintBattery(painter, stackOffsets);
}

void DesktopOverlayWindow::paintClock(QPainter& painter, QHash<int, int>& stackOffsets)
{
    if (!m_settings.clockEnabled)
        return;

    const QString family = m_settings.clockFontFamily.isEmpty() ? font().family() : m_settings.clockFontFamily;
    QFont mainFont(family, m_settings.clockFontSize, QFont::DemiBold);

    const QTime now = QTime::currentTime();

    if (m_settings.clockLayout == ClockLayout::Analog) {
        const int diameter = qMax(90, m_settings.clockFontSize * 3);
        const QRect rect = computeAnchorRect(m_settings.clockPosition, m_settings.clockMargin,
                                              QSize(diameter, diameter), stackOffsets);
        paintAnalogClock(painter, rect, now, m_settings.clockTheme);
        return;
    }

    const QString timeText = now.toString(QStringLiteral("HH:mm"));
    QString dateText;
    if (m_settings.clockLayout == ClockLayout::DigitalWithDate)
        dateText = QDate::currentDate().toString(QStringLiteral("dddd, MMMM d"));

    const QFontMetrics timeMetrics(mainFont);
    QFont dateFont = mainFont;
    dateFont.setPointSize(qMax(8, m_settings.clockFontSize / 3));
    const QFontMetrics dateMetrics(dateFont);

    int width = timeMetrics.horizontalAdvance(timeText);
    int height = timeMetrics.height();
    if (!dateText.isEmpty()) {
        width = qMax(width, dateMetrics.horizontalAdvance(dateText));
        height += dateMetrics.height() + 4;
    }

    const QRect rect = computeAnchorRect(m_settings.clockPosition, m_settings.clockMargin, QSize(width, height),
                                          stackOffsets);

    painter.save();
    painter.translate(rect.center());
    painter.rotate(m_settings.clockRotation);
    painter.translate(-rect.center());

    drawStyledText(painter, QRect(rect.x(), rect.y(), width, timeMetrics.height()), Qt::AlignHCenter | Qt::AlignTop,
                    mainFont, timeText, m_settings.clockTheme);
    if (!dateText.isEmpty()) {
        drawStyledText(painter, QRect(rect.x(), rect.y() + timeMetrics.height() + 4, width, dateMetrics.height()),
                        Qt::AlignHCenter | Qt::AlignTop, dateFont, dateText, m_settings.clockTheme);
    }

    painter.restore();
}

void DesktopOverlayWindow::paintAnalogClock(QPainter& painter, const QRect& rect, const QTime& time,
                                             OverlayTheme theme)
{
    painter.save();

    const QColor color = theme == OverlayTheme::Outline ? themeOutlineColor(theme) : themeTextColor(theme);
    QPen facePen(color, 2);
    painter.setPen(facePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(rect);

    const QPointF center = rect.center();
    const double radius = rect.width() / 2.0;

    for (int i = 0; i < 12; ++i) {
        const double angle = i * 30.0 * kDegToRad;
        const QPointF outer(center.x() + radius * 0.9 * std::sin(angle), center.y() - radius * 0.9 * std::cos(angle));
        const QPointF inner(center.x() + radius * 0.78 * std::sin(angle),
                             center.y() - radius * 0.78 * std::cos(angle));
        painter.drawLine(inner, outer);
    }

    auto drawHand = [&](double angleDeg, double lengthFactor, double width) {
        const double angle = angleDeg * kDegToRad;
        const QPointF end(center.x() + radius * lengthFactor * std::sin(angle),
                           center.y() - radius * lengthFactor * std::cos(angle));
        QPen pen(color, width);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawLine(center, end);
    };

    const double hourAngle = (time.hour() % 12 + time.minute() / 60.0) * 30.0;
    const double minuteAngle = (time.minute() + time.second() / 60.0) * 6.0;
    drawHand(hourAngle, 0.5, 4.0);
    drawHand(minuteAngle, 0.72, 3.0);

    painter.restore();
}

void DesktopOverlayWindow::paintCalendar(QPainter& painter, QHash<int, int>& stackOffsets)
{
    if (!m_settings.calendarEnabled)
        return;

    const QDate today = QDate::currentDate();
    const int cellSize = 26;
    const int firstOfMonth = QDate(today.year(), today.month(), 1).dayOfWeek() % 7; // Sunday = 0
    const int daysInMonth = today.daysInMonth();
    const int rows = (firstOfMonth + daysInMonth + 6) / 7;

    const QSize size(cellSize * 7, cellSize * (rows + 2));
    const QRect rect =
        computeAnchorRect(m_settings.calendarPosition, m_settings.calendarMargin, size, stackOffsets);

    const OverlayTheme theme = m_settings.calendarTheme;
    QFont headerFont = font();
    headerFont.setBold(true);
    headerFont.setPointSize(11);
    drawStyledText(painter, QRect(rect.x(), rect.y(), rect.width(), cellSize), Qt::AlignCenter, headerFont,
                    today.toString(QStringLiteral("MMMM yyyy")), theme);

    QFont dayNameFont = font();
    dayNameFont.setPointSize(9);
    static const char* kDayNames[] = {"S", "M", "T", "W", "T", "F", "S"};
    for (int col = 0; col < 7; ++col) {
        drawStyledText(painter, QRect(rect.x() + col * cellSize, rect.y() + cellSize, cellSize, cellSize),
                        Qt::AlignCenter, dayNameFont, QString::fromLatin1(kDayNames[col]), theme);
    }

    QFont dayFont = font();
    dayFont.setPointSize(9);
    int dayNumber = 1;
    for (int cell = firstOfMonth; dayNumber <= daysInMonth; ++cell) {
        const int row = cell / 7;
        const int col = cell % 7;
        const QRect cellRect(rect.x() + col * cellSize, rect.y() + cellSize * 2 + row * cellSize, cellSize, cellSize);

        if (dayNumber == today.day()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0x5b, 0xa8, 0xe6));
            painter.drawEllipse(cellRect.adjusted(3, 3, -3, -3));
            painter.setFont(dayFont);
            painter.setPen(Qt::white);
            painter.drawText(cellRect, Qt::AlignCenter, QString::number(dayNumber));
        } else {
            drawStyledText(painter, cellRect, Qt::AlignCenter, dayFont, QString::number(dayNumber), theme);
        }
        ++dayNumber;
    }
}

void DesktopOverlayWindow::paintBattery(QPainter& painter, QHash<int, int>& stackOffsets)
{
    if (!m_settings.batteryIndicatorEnabled)
        return;

    QFont textFont = font();
    textFont.setPointSize(10);
    const QFontMetrics metrics(textFont);

    QStringList lines;
    if (m_batteryDevices.isEmpty()) {
        lines << QStringLiteral("No Bluetooth battery devices found");
    } else {
        for (const BluetoothDeviceBattery& device : m_batteryDevices)
            lines << QStringLiteral("%1: %2%").arg(device.name).arg(device.batteryPercent);
    }

    int width = 0;
    for (const QString& line : lines)
        width = qMax(width, metrics.horizontalAdvance(line));
    const int height = metrics.height() * lines.size();

    const QRect rect =
        computeAnchorRect(m_settings.batteryPosition, m_settings.batteryMargin, QSize(width, height), stackOffsets);

    for (int i = 0; i < lines.size(); ++i) {
        drawStyledText(painter, QRect(rect.x(), rect.y() + i * metrics.height(), width, metrics.height()),
                        Qt::AlignLeft | Qt::AlignTop, textFont, lines.at(i), m_settings.batteryTheme);
    }
}

} // namespace colorfy
