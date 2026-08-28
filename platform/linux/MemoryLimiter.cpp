#include "MemoryLimiter.h"

#include <QDebug>

#include <sys/resource.h>

namespace colorfy {

bool MemoryLimiter::apply(int limitMb)
{
    if (limitMb <= 0)
        return false;

    struct rlimit limit;
    limit.rlim_cur = static_cast<rlim_t>(limitMb) * 1024ULL * 1024ULL;
    limit.rlim_max = limit.rlim_cur;

    if (setrlimit(RLIMIT_AS, &limit) != 0) {
        qWarning() << "MemoryLimiter: setrlimit(RLIMIT_AS) failed";
        return false;
    }

    return true;
}

} // namespace colorfy
