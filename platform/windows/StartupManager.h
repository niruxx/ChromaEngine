#pragma once

namespace colorfy {

// Toggles launching the app at Windows login via the per-user
// HKCU\...\CurrentVersion\Run registry key. No admin rights needed, fully
// reversible (just deletes the value when disabled).
class StartupManager {
public:
    static bool isEnabled();
    static void setEnabled(bool enabled);
};

} // namespace colorfy
