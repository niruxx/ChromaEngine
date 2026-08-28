#pragma once

#include <QString>
#include <QStringList>

namespace colorfy {

enum class FitMode {
    Fill,
    Fit,
    Stretch,
    Center,
    Free,
};

// Shared by clock/calendar/battery overlay widgets.
enum class OverlayPosition {
    TopLeft = 0,
    TopRight = 1,
    BottomLeft = 2,
    BottomRight = 3,
    Center = 4,
};

// Visual style for overlay widgets (clock/calendar/battery) - independent of
// the library window's own dark theme, since these render over arbitrary
// video content and need to stay legible regardless of what's playing.
enum class OverlayTheme {
    Light = 0,
    Dark = 1,
    Accent = 2,
    Outline = 3,
};

enum class ClockLayout {
    Digital = 0,
    DigitalWithDate = 1,
    Analog = 2,
};

struct MediaItem {
    QString filePath;
    QString libraryFolder;
    bool muted = true;
    int volume = 100;
    FitMode fitMode = FitMode::Fill;

    bool flipHorizontal = false;
    bool flipVertical = false;
    int brightness = 0; // -100..100
    int contrast = 0; // -100..100
    int saturation = 0; // -100..100
    double playbackRate = 1.0; // 0.25..3.0
    double zoom = 0.0; // log2 scale, Free fit mode only

    bool launchAtStartup = false;
    bool closeToTray = true;
    bool startMinimized = false; // false = show the library window on launch, foregrounded
    int backgroundTheme = 0; // 0=None, 1=Aurora, 2=Starfield (see ui/AnimatedBackground.h)
    bool showDesktopIcons = true; // Windows only: wallpaper behind icons vs. covering them

    bool memoryLimitEnabled = false;
    int memoryLimitMb = 4096; // hard cap via Windows Job Object, applied at process startup

    // Forces mpv onto a plain, GPU-less rendering path (see MpvSurface.cpp)
    // instead of its default GPU-accelerated one - for VMs/remote desktops/
    // old hardware where GPU context creation is missing, broken, or flaky.
    // Applied when each mpv instance is created, so - like the memory limit
    // above - takes effect the next time the app starts.
    bool softwareRendering = false;

    int previewFrameRateLimit = 0; // fps, 0 = unlimited; preview pane only, not the desktop wallpaper

    // Off by default: cycling every tile's thumbnail through several frames
    // multiplies both generation cost (up to 6x mpv screenshots per video
    // instead of 1) and steady-state repaint work, and was a major factor
    // in thumbnail generation feeling slow/heavy on large folders.
    bool thumbnailAutoPlayEnabled = false;

    // --- Desktop overlay widgets (platform/windows/DesktopOverlayWindow) ---
    bool clockEnabled = false;
    ClockLayout clockLayout = ClockLayout::Digital;
    OverlayTheme clockTheme = OverlayTheme::Light;
    QString clockFontFamily; // empty = system default
    int clockFontSize = 48;
    int clockRotation = 0; // 0/90/180/270, for portrait monitors
    OverlayPosition clockPosition = OverlayPosition::TopRight;
    int clockMargin = 40;

    bool calendarEnabled = false;
    OverlayTheme calendarTheme = OverlayTheme::Light;
    OverlayPosition calendarPosition = OverlayPosition::BottomRight;
    int calendarMargin = 40;

    bool batteryIndicatorEnabled = false;
    OverlayTheme batteryTheme = OverlayTheme::Light;
    OverlayPosition batteryPosition = OverlayPosition::TopLeft;
    int batteryMargin = 40;

    // Monitor device names the wallpaper should be applied to. Empty means
    // "primary monitor only" (the original single-monitor behavior).
    QStringList enabledMonitorIds;
};

} // namespace colorfy
