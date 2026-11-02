#include "ProcessManager.hpp"
#include <iostream>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#endif

namespace orca::orchestrator {

ProcessManager::ProcessManager() {
#if defined(_WIN32)
    // Windows Job Object ensures all child sidecar processes terminate if the orchestrator terminates
    m_job_object = CreateJobObjectW(NULL, NULL);
    if (m_job_object) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(m_job_object, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
    }
#endif
}

ProcessManager::~ProcessManager() {
#if defined(_WIN32)
    if (m_job_object) {
        CloseHandle(static_cast<HANDLE>(m_job_object));
        m_job_object = nullptr;
    }
#endif
}

compat::expected<ProcessInfo, std::string> ProcessManager::spawn(
    const std::string &executable_path,
    const std::vector<std::string> &args,
    const fs::path &working_dir,
    const std::vector<std::pair<std::string, std::string>> &env_vars)
{
    if (!fs::exists(executable_path)) {
        return compat::unexpected("Executable path does not exist: " + executable_path);
    }

#if defined(_WIN32)
    std::wstring wcmd = L"\"" + fs::path(executable_path).wstring() + L"\"";
    for (const auto &arg : args) {
        wcmd += L" \"" + fs::path(arg).wstring() + L"\"";
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::wstring wdir = working_dir.wstring();
    const wchar_t* p_wdir = wdir.empty() ? nullptr : wdir.c_str();

    BOOL success = CreateProcessW(
        NULL,
        &wcmd[0],
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE, // Launch in independent console/window
        NULL,
        p_wdir,
        &si,
        &pi
    );

    if (!success) {
        DWORD err = GetLastError();
        return compat::unexpected("Failed to CreateProcess. Error code: " + std::to_string(err));
    }

    if (m_job_object) {
        AssignProcessToJobObject(static_cast<HANDLE>(m_job_object), pi.hProcess);
    }

    CloseHandle(pi.hThread); // Thread handle not needed

    ProcessInfo info;
    info.pid = static_cast<uint32_t>(pi.dwProcessId);
    info.name = fs::path(executable_path).stem().string();
    info.is_alive = true;
    info.native_handle = pi.hProcess;
    return info;

#else
    // POSIX / Linux implementation
    pid_t pid = fork();
    if (pid < 0) {
        return compat::unexpected("fork() failed");
    }

    if (pid == 0) {
        // Child process
        if (!working_dir.empty()) {
            chdir(working_dir.c_str());
        }

        std::vector<char*> c_args;
        c_args.push_back(const_cast<char*>(executable_path.c_str()));
        for (const auto &a : args) {
            c_args.push_back(const_cast<char*>(a.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(executable_path.c_str(), c_args.data());
        _exit(127);
    }

    ProcessInfo info;
    info.pid = static_cast<uint32_t>(pid);
    info.name = fs::path(executable_path).stem().string();
    info.is_alive = true;
    return info;
#endif
}

bool ProcessManager::is_alive(ProcessInfo &info) {
    if (!info.is_alive || info.pid == 0) return false;

#if defined(_WIN32)
    if (!info.native_handle) return false;
    DWORD exit_code = 0;
    if (GetExitCodeProcess(static_cast<HANDLE>(info.native_handle), &exit_code)) {
        if (exit_code == STILL_ACTIVE) {
            return true;
        }
    }
    info.is_alive = false;
    return false;
#else
    int status;
    pid_t res = waitpid(static_cast<pid_t>(info.pid), &status, WNOHANG);
    if (res == 0) {
        return true;
    }
    info.is_alive = false;
    return false;
#endif
}

bool ProcessManager::terminate(ProcessInfo &info, std::chrono::milliseconds timeout) {
    if (!is_alive(info)) return true;

#if defined(_WIN32)
    if (!info.native_handle) return true;
    // Request termination
    TerminateProcess(static_cast<HANDLE>(info.native_handle), 0);
    DWORD wait_res = WaitForSingleObject(static_cast<HANDLE>(info.native_handle), static_cast<DWORD>(timeout.count()));
    CloseHandle(static_cast<HANDLE>(info.native_handle));
    info.native_handle = nullptr;
    info.is_alive = false;
    return (wait_res == WAIT_OBJECT_0);
#else
    kill(static_cast<pid_t>(info.pid), SIGTERM);
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < timeout) {
        if (!is_alive(info)) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return kill(info);
#endif
}

bool ProcessManager::kill(ProcessInfo &info) {
#if defined(_WIN32)
    return terminate(info, std::chrono::milliseconds(0));
#else
    if (info.pid != 0) {
        ::kill(static_cast<pid_t>(info.pid), SIGKILL);
        info.is_alive = false;
        return true;
    }
    return false;
#endif
}

} // namespace orca::orchestrator
