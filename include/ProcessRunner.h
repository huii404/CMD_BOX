#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>
#include <vector>

namespace ProcessRunner {
inline bool run(const std::string& command, bool hide = true, DWORD timeout = 45 * 60 * 1000) {
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, command.c_str(), -1, nullptr, 0);
    if (!count) return false;
    std::vector<wchar_t> line(count);
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, command.c_str(), -1, line.data(), count)) return false;
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (!job) return false;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        CloseHandle(job); return false;
    }
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    DWORD flags = CREATE_SUSPENDED | (hide ? CREATE_NO_WINDOW : 0);
    if (!CreateProcessW(nullptr, line.data(), nullptr, nullptr, FALSE, flags, nullptr, nullptr, &startup, &process)) {
        CloseHandle(job); return false;
    }
    bool ok = false;
    if (AssignProcessToJobObject(job, process.hProcess) && ResumeThread(process.hThread) != DWORD(-1)) {
        if (WaitForSingleObject(process.hProcess, timeout) == WAIT_OBJECT_0) {
            DWORD exit = 1;
            ok = GetExitCodeProcess(process.hProcess, &exit) && exit == 0;
        } else {
            TerminateJobObject(job, 1);
            WaitForSingleObject(process.hProcess, 5000);
        }
    } else {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, 5000);
    }
    CloseHandle(process.hThread); CloseHandle(process.hProcess); CloseHandle(job);
    return ok;
}
}
