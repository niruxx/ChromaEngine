#pragma once

namespace colorfy {

// Hard-caps this process's committed memory via a Windows Job Object
// (JOB_OBJECT_LIMIT_JOB_MEMORY). Once applied, Windows itself kills the
// process if it exceeds the limit - there is no user-mode way to lower or
// remove a job's memory limit while the process stays in that job, so this
// is only meant to be applied once, at startup, based on the saved setting.
class MemoryLimiter {
public:
    // Returns false if the limit could not be applied (e.g. the process is
    // already in an incompatible job, such as a container/sandbox).
    static bool apply(int limitMb);
};

} // namespace colorfy
