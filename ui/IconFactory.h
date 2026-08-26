#pragma once

#include <QIcon>

namespace colorfy::IconFactory {

// Small, flat, procedurally-drawn icon set (no external assets) used
// throughout the toolbar, sidebar actions, and tray menu for a consistent
// look instead of the OS's default QStyle icons.
QIcon folder(int size = 20);
QIcon refresh(int size = 20);
QIcon play(int size = 20);
QIcon pause(int size = 20);
QIcon gear(int size = 20);
QIcon monitor(int size = 20);
QIcon rename(int size = 20);
QIcon trash(int size = 20);
QIcon applyCheck(int size = 20);
QIcon flipHorizontal(int size = 20);
QIcon flipVertical(int size = 20);
QIcon appLogo(int size = 256);

// Title bar window controls.
QIcon windowMinimize(int size = 16);
QIcon windowMaximize(int size = 16);
QIcon windowRestore(int size = 16);
QIcon windowClose(int size = 16);

} // namespace colorfy::IconFactory
