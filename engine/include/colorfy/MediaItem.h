#pragma once

#include <QString>

namespace colorfy {

enum class FitMode {
    Fill,
    Fit,
    Stretch,
    Center,
};

struct MediaItem {
    QString filePath;
    QString libraryFolder;
    bool muted = true;
    int volume = 100;
    FitMode fitMode = FitMode::Fill;
};

} // namespace colorfy
