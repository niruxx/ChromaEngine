#pragma once

namespace colorfy {

// Hard-caps this process's virtual address space via setrlimit(RLIMIT_AS).
// The kernel fails further allocations once the limit is hit (malloc returns
// null / new throws bad_alloc) rather than actively killing the process the
// way Windows' Job Object limit does - the practical effect for this app is
// the same, since an allocation failure here isn't something the app tries
// to recover from either.
class MemoryLimiter {
public:
    // Returns false if the limit could not be applied.
    static bool apply(int limitMb);
};

} // namespace colorfy
