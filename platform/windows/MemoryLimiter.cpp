#include "MemoryLimiter.h"

#include <QDebug>

#include <windows.h>

namespace colorfy {

bool MemoryLimiter::apply(int limitMb)
{
    if (limitMb <= 0)
        return false;

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (!job) {
        qWarning() << "MemoryLimiter: CreateJobObjectW failed, error" << GetLastError();
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limitInfo{};
    limitInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_MEMORY;
    limitInfo.JobMemoryLimit = static_cast<SIZE_T>(limitMb) * 1024ULL * 1024ULL;

    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limitInfo, sizeof(limitInfo))) {
        qWarning() << "MemoryLimiter: SetInformationJobObject failed, error" << GetLastError();
        CloseHandle(job);
        return false;
    }

    if (!AssignProcessToJobObject(job, GetCurrentProcess())) {
        // Most commonly happens if the process is already in another job
        // (e.g. launched under a sandbox or a job-managed shell) that
        // doesn't allow nesting on this Windows version.
        qWarning() << "MemoryLimiter: AssignProcessToJobObject failed, error" << GetLastError();
        CloseHandle(job);
        return false;
    }

    // Intentionally leak the handle: the job must outlive this function for
    // its limit to stay in effect, and it's cleaned up automatically when
    // the process exits.
    return true;
}

} // namespace colorfy
