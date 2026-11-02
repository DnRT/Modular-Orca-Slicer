#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>

#include "ProcessManager.hpp"
#include "FrankensteinProfile.hpp"

namespace fs = std::filesystem;
using namespace orca::orchestrator;

void print_banner() {
    std::cout << "=================================================================\n";
    std::cout << "  ORCASLICER MODULAR NATIVE ORCHESTRATOR (C++23 / C++20)\n";
    std::cout << "  Multi-Environment Supervisor & CurseForge Maker System\n";
    std::cout << "=================================================================\n\n";
}

void cmd_list(const fs::path &base_dir) {
    FrankensteinManager mgr(base_dir);
    auto profiles = mgr.list_profiles();

    std::cout << "Registered Frankenstein Instances (" << profiles.size() << " found):\n";
    std::cout << "-----------------------------------------------------------------\n";
    if (profiles.empty()) {
        std::cout << "  No profiles found. Create one with: OrcaOrchestrator create <id> --name \"Name\"\n";
    }

    for (const auto &p : profiles) {
        std::cout << " • ID:          " << p.id << "\n";
        std::cout << "   Name:        " << p.name << "\n";
        std::cout << "   Description: " << (p.description.empty() ? "(none)" : p.description) << "\n";
        std::cout << "   DataDir:     " << p.get_data_dir(base_dir).string() << "\n";
        std::cout << "   Plugins:     ";
        if (p.active_plugins.empty()) {
            std::cout << "(none)\n";
        } else {
            for (size_t i = 0; i < p.active_plugins.size(); ++i) {
                std::cout << p.active_plugins[i] << (i + 1 < p.active_plugins.size() ? ", " : "\n");
            }
        }
        std::cout << "-----------------------------------------------------------------\n";
    }
}

int cmd_launch(const fs::path &base_dir, const std::string &profile_id) {
    FrankensteinManager mgr(base_dir);
    auto profile_opt = mgr.get_profile(profile_id);
    if (!profile_opt) {
        std::cerr << "Error: Profile '" << profile_id << "' not found.\n";
        return 1;
    }

    auto &p = *profile_opt;
    fs::path data_dir = p.get_data_dir(base_dir);
    fs::create_directories(data_dir);

    ProcessManager proc_mgr;

    std::cout << ">>> Launching Frankenstein Profile: " << p.name << " (" << p.id << ")\n";
    std::cout << "    Isolated DataDir: " << data_dir.string() << "\n";
    std::cout << "    Slicer Executable: " << p.slicer_bin << "\n";

    std::vector<std::string> args = {"--datadir", data_dir.string()};
    for (const auto &extra : p.extra_args) {
        args.push_back(extra);
    }

    auto spawn_res = proc_mgr.spawn(p.slicer_bin, args, base_dir);
    if (!spawn_res) {
        std::cerr << "Launch failed: " << spawn_res.error() << "\n";
        return 1;
    }

    auto proc = spawn_res.value();
    std::cout << ">>> OrcaSlicer running successfully! (PID: " << proc.pid << ")\n";
    std::cout << ">>> Waiting for session to close...\n";

    while (proc_mgr.is_alive(proc)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << ">>> Session ended cleanly.\n";
    return 0;
}

int cmd_test_dual(const fs::path &base_dir, int duration_sec) {
    FrankensteinManager mgr(base_dir);
    ProcessManager proc_mgr;

    std::string bin = R"(C:\Program Files\OrcaSlicer\orca-slicer.exe)";
    fs::path dir1 = base_dir / "environments" / "test-alpha" / "data";
    fs::path dir2 = base_dir / "environments" / "test-beta" / "data";
    fs::create_directories(dir1);
    fs::create_directories(dir2);

    std::cout << ">>> [Native C++23 Dual Test] Spawning Instance 1 (Alpha)...\n";
    auto res1 = proc_mgr.spawn(bin, {"--datadir", dir1.string()}, base_dir);
    if (!res1) {
        std::cerr << "Failed to spawn Instance 1: " << res1.error() << "\n";
        return 1;
    }
    auto p1 = res1.value();
    std::cout << "    Instance 1 running (PID: " << p1.pid << ", Data: " << dir1.string() << ")\n\n";

    std::cout << ">>> [Native C++23 Dual Test] Spawning Instance 2 (Beta)...\n";
    auto res2 = proc_mgr.spawn(bin, {"--datadir", dir2.string()}, base_dir);
    if (!res2) {
        std::cerr << "Failed to spawn Instance 2: " << res2.error() << "\n";
        proc_mgr.terminate(p1);
        return 1;
    }
    auto p2 = res2.value();
    std::cout << "    Instance 2 running (PID: " << p2.pid << ", Data: " << dir2.string() << ")\n\n";

    std::cout << ">>> Both native instances active! Monitoring concurrently for " << duration_sec << " seconds:\n";
    for (int t = 1; t <= duration_sec; ++t) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        bool a1 = proc_mgr.is_alive(p1);
        bool a2 = proc_mgr.is_alive(p2);
        std::cout << "  [T+" << (t < 10 ? "0" : "") << t << "s] Instance 1 (PID " << p1.pid << ") Alive: " 
                  << (a1 ? "YES" : "NO") << " | Instance 2 (PID " << p2.pid << ") Alive: " 
                  << (a2 ? "YES" : "NO") << "\n";
        if (!a1 || !a2) {
            std::cout << "Warning: A process terminated early.\n";
            break;
        }
    }

    std::cout << "\n>>> [Native C++23 Dual Test] Teardown: Cleanly terminating all processes...\n";
    proc_mgr.terminate(p1);
    proc_mgr.terminate(p2);
    std::cout << ">>> All native environments terminated cleanly.\n";
    return 0;
}

int main(int argc, char* argv[]) {
    print_banner();

    fs::path base_dir;
    try {
        base_dir = fs::canonical(fs::path(argv[0]).parent_path());
    } catch (...) {
        base_dir = fs::current_path();
    }
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  OrcaOrchestrator list\n";
        std::cout << "  OrcaOrchestrator launch <profile-id>\n";
        std::cout << "  OrcaOrchestrator create <profile-id> --name \"Name\" [--desc \"Desc\"]\n";
        std::cout << "  OrcaOrchestrator test-dual [--duration 5]\n";
        return 0;
    }

    std::string cmd = argv[1];
    if (cmd == "list") {
        cmd_list(base_dir);
        return 0;
    } else if (cmd == "launch") {
        if (argc < 3) {
            std::cerr << "Error: Missing profile ID. Usage: OrcaOrchestrator launch <profile-id>\n";
            return 1;
        }
        return cmd_launch(base_dir, argv[2]);
    } else if (cmd == "create") {
        if (argc < 3) {
            std::cerr << "Error: Missing profile ID. Usage: OrcaOrchestrator create <id> --name \"Name\"\n";
            return 1;
        }
        std::string id = argv[2];
        std::string name = id;
        std::string desc = "";
        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--name" && i + 1 < argc) {
                name = argv[++i];
            } else if (arg == "--desc" && i + 1 < argc) {
                desc = argv[++i];
            }
        }
        FrankensteinManager mgr(base_dir);
        auto res = mgr.create_profile(id, name, desc);
        if (!res) {
            std::cerr << "Error creating profile: " << res.error() << "\n";
            return 1;
        }
        std::cout << "Profile '" << name << "' (" << id << ") created successfully!\n";
        return 0;
    } else if (cmd == "test-dual") {
        int duration = 5;
        if (argc >= 4 && std::string(argv[2]) == "--duration") {
            duration = std::stoi(argv[3]);
        }
        return cmd_test_dual(base_dir, duration);
    } else {
        std::cerr << "Unknown command: " << cmd << "\n";
        return 1;
    }
}
