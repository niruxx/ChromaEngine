#pragma once

namespace colorfy {

// Toggles launching the app at login via an XDG autostart .desktop file in
// ~/.config/autostart/ - the standard mechanism honored by XFCE, MATE,
// GNOME, and KDE alike. No special permissions needed, fully reversible
// (just deletes the file when disabled).
class StartupManager {
public:
    static bool isEnabled();
    static void setEnabled(bool enabled);
};

} // namespace colorfy
