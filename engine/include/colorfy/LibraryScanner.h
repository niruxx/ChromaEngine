#pragma once

#include <QStringList>

namespace colorfy {

class LibraryScanner {
public:
    // Top-level video/GIF files in `folder`, sorted by name. Non-recursive.
    static QStringList scan(const QString& folder);

    static bool isGif(const QString& filePath);
};

} // namespace colorfy
