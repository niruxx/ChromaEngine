#pragma once

#include "colorfy/MediaItem.h"

namespace colorfy {

class ConfigStore {
public:
    static MediaItem load();
    static void save(const MediaItem& item);

private:
    static QString configFilePath();
};

} // namespace colorfy
