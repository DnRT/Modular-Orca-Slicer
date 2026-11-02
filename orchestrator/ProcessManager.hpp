#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <filesystem>
#include "compat/expected.hpp"

namespace orca::orchestrator {

namespace fs = std::filesystem;

struct ProcessInfo {
    uint32_t pid{0};
    std::string name;
    bool is_alive{false};
#if defined(_WIN32)
    void* native_handle{nullptr}; // HANDLE
#endif
};

class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();

    // Spawn an external process (Windows & Linux cross-platform)
    compat::expected<ProcessInfo, std::string> spawn(
        const std::string &executable_path,
        const std::vector<std::string> &args,
        const fs::path &working_dir,
        const std::vector<std::pair<std::string, std::string>> &env_vars = {}
    );

    // Check if process is still running
    bool is_alive(ProcessInfo &info);

    // Terminate process safely
    bool terminate(ProcessInfo &info, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));

    // Force kill
    bool kill(ProcessInfo &info);

private:
#if defined(_WIN32)
    void* m_job_object{nullptr}; // Windows JobObject to guarantee cleanup of child processes
#endif
};

} // namespace orca::orchestrator
