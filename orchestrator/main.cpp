#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include <iomanip>

#include "ProcessManager.hpp"
#include "FrankensteinProfile.hpp"

namespace fs = std::filesystem;
using namespace orca::orchestrator;

void print_banner() {
    std::cout << "=================================================================\n";
    std::cout << "  ORCASLICER MODULAR NATIVE ORCHESTRATOR (C++23 / C++20)\n";
    std::cout << "  CurseForge-Style Modpack Composer & Ecosystem Supervisor\n";
    std::cout << "=================================================================\n\n";
}

void print_help() {
    std::cout << "Usage:\n";
    std::cout << "  OrcaOrchestrator interactive                        Run interactive TUI menu\n";
    std::cout << "  OrcaOrchestrator mods list                          List available mods in ecosystem\n";
    std::cout << "  OrcaOrchestrator mods info <mod_id>                 Show mod specifications & hooks\n";
    std::cout << "  OrcaOrchestrator modpack list                       List created Frankenstein modpacks\n";
    std::cout << "  OrcaOrchestrator modpack info <pack_id>             Inspect modpack & active mod list\n";
    std::cout << "  OrcaOrchestrator modpack create <id> [--name <n>]   Create a new Frankenstein instance\n";
    std::cout << "  OrcaOrchestrator modpack add <pack_id> <mod_id>     Add community mod to modpack\n";
    std::cout << "  OrcaOrchestrator modpack remove <pack_id> <mod_id>  Remove mod from modpack\n";
    std::cout << "  OrcaOrchestrator launch <pack_id> [extra args...]   Compile manifest & launch OrcaSlicer\n";
    std::cout << "  OrcaOrchestrator test-dual [--duration <sec>]       Run parallel instance sandbox test\n\n";
}

void cmd_mods_list(FrankensteinManager &mgr) {
    auto mods = mgr.list_available_mods();
    std::cout << "=================================================================\n";
    std::cout << "  AVAILABLE COMMUNITY MODS IN ECOSYSTEM (" << mods.size() << " registered)\n";
    std::cout << "=================================================================\n";
    for (const auto &m : mods) {
        std::cout << " [MOD] " << m.id << "\n";
        std::cout << "   Name:           " << m.name << "\n";
        std::cout << "   Subsystem Hook: [" << m.target_hook << "]\n";
        std::cout << "   Author:         " << m.author << "\n";
        std::cout << "   Description:    " << m.description << "\n";
        std::cout << "-----------------------------------------------------------------\n";
    }
}

void cmd_mods_info(FrankensteinManager &mgr, const std::string &mod_id) {
    auto m_opt = mgr.get_mod(mod_id);
    if (!m_opt) {
        std::cerr << "Error: Mod '" << mod_id << "' not found in catalog.\n";
        return;
    }
    const auto &m = *m_opt;
    std::cout << "=================================================================\n";
    std::cout << "  MOD SPECIFICATION: " << m.name << "\n";
    std::cout << "=================================================================\n";
    std::cout << "  Mod ID:         " << m.id << "\n";
    std::cout << "  Replaces Hook:  [" << m.target_hook << "]\n";
    std::cout << "  Author:         " << m.author << "\n";
    std::cout << "  Binary Source:  " << (m.library_path.empty() ? "(Built-in Modular C++ Strategy)" : m.library_path) << "\n";
    std::cout << "  Description:    " << m.description << "\n";
    std::cout << "=================================================================\n";
}

void cmd_modpack_list(FrankensteinManager &mgr, const fs::path &base_dir) {
    auto packs = mgr.list_modpacks();
    std::cout << "=================================================================\n";
    std::cout << "  FRANKENSTEIN MODPACK PROFILES (" << packs.size() << " configured)\n";
    std::cout << "=================================================================\n";
    if (packs.empty()) {
        std::cout << "  No modpack profiles found.\n";
        std::cout << "  Create your first modpack with:\n";
        std::cout << "    OrcaOrchestrator modpack create my-frankenstein --name \"My Custom Orca\"\n";
    }
    for (const auto &p : packs) {
        std::cout << " • ID:          " << p.id << "\n";
        std::cout << "   Name:        " << p.name << "\n";
        std::cout << "   Author:      " << p.author << "\n";
        std::cout << "   DataDir:     " << p.get_data_dir(base_dir).string() << "\n";
        std::cout << "   Active Mods: " << p.mods.size() << "\n";
        for (const auto &m : p.mods) {
            std::cout << "     + [" << m.target_hook << "] -> " << m.name << " (" << m.id << ")\n";
        }
        std::cout << "-----------------------------------------------------------------\n";
    }
}

void cmd_modpack_info(FrankensteinManager &mgr, const fs::path &base_dir, const std::string &pack_id) {
    auto pack_opt = mgr.get_modpack(pack_id);
    if (!pack_opt) {
        std::cerr << "Error: Modpack '" << pack_id << "' not found.\n";
        return;
    }
    const auto &p = *pack_opt;
    std::cout << "=================================================================\n";
    std::cout << "  MODPACK PROFILE: " << p.name << " [" << p.id << "]\n";
    std::cout << "=================================================================\n";
    std::cout << "  Author:          " << p.author << "\n";
    std::cout << "  Description:     " << (p.description.empty() ? "(none)" : p.description) << "\n";
    std::cout << "  Isolated Data:   " << p.get_data_dir(base_dir).string() << "\n";
    std::cout << "  Manifest Output: " << p.get_modpack_manifest_path(base_dir).string() << "\n";
    std::cout << "  Slicer Exec:     " << p.slicer_bin << "\n";
    std::cout << "\n  Installed Community Mods (" << p.mods.size() << "):\n";
    if (p.mods.empty()) {
        std::cout << "    (Standard base OrcaSlicer - no subsystems replaced)\n";
    } else {
        for (size_t i = 0; i < p.mods.size(); ++i) {
            const auto &m = p.mods[i];
            std::cout << "    " << (i + 1) << ". " << m.name << " [" << m.id << "]\n";
            std::cout << "       Intercepts Hook: [" << m.target_hook << "]\n";
            std::cout << "       State:           " << (m.enabled ? "ACTIVE" : "DISABLED") << "\n";
        }
    }
    std::cout << "=================================================================\n";
}

int cmd_modpack_create(FrankensteinManager &mgr, const std::string &id, const std::string &name, const std::string &desc) {
    auto res = mgr.create_modpack(id, name, desc);
    if (!res) {
        std::cerr << "Failed to create modpack: " << res.error() << "\n";
        return 1;
    }
    std::cout << ">>> Created Frankenstein modpack '" << name << "' [" << id << "] successfully!\n";
    std::cout << ">>> Add mods with: OrcaOrchestrator modpack add " << id << " wave_overhangs\n";
    return 0;
}

int cmd_modpack_add(FrankensteinManager &mgr, const std::string &pack_id, const std::string &mod_id) {
    auto res = mgr.add_mod(pack_id, mod_id);
    if (!res) {
        std::cerr << "Error adding mod: " << res.error() << "\n";
        return 1;
    }
    std::cout << ">>> Added mod '" << mod_id << "' to modpack '" << pack_id << "' successfully!\n";
    return 0;
}

int cmd_modpack_remove(FrankensteinManager &mgr, const std::string &pack_id, const std::string &mod_id) {
    auto res = mgr.remove_mod(pack_id, mod_id);
    if (!res) {
        std::cerr << "Error removing mod: " << res.error() << "\n";
        return 1;
    }
    std::cout << ">>> Removed mod '" << mod_id << "' from modpack '" << pack_id << "'.\n";
    return 0;
}

int cmd_launch(FrankensteinManager &mgr, const fs::path &base_dir, const std::string &pack_id, const std::vector<std::string> &extra_args) {
    auto pack_opt = mgr.get_modpack(pack_id);
    if (!pack_opt) {
        std::cerr << "Error: Modpack '" << pack_id << "' not found.\n";
        return 1;
    }
    auto &p = *pack_opt;

    // 1. Generate runtime modpack.json manifest
    auto manifest_res = mgr.generate_manifest(pack_id);
    if (!manifest_res) {
        std::cerr << "Error generating modpack manifest: " << manifest_res.error() << "\n";
        return 1;
    }
    fs::path manifest_path = manifest_res.value();
    fs::path data_dir = p.get_data_dir(base_dir);
    fs::create_directories(data_dir);

    // Also copy manifest to data_dir/modpack.json for automated detection
    try {
        fs::copy_file(manifest_path, data_dir / "modpack.json", fs::copy_options::overwrite_existing);
    } catch (...) {}

    std::cout << "=================================================================\n";
    std::cout << "  LAUNCHING FRANKENSTEIN MODPACK: " << p.name << " [" << p.id << "]\n";
    std::cout << "=================================================================\n";
    std::cout << "  Isolated DataDir: " << data_dir.string() << "\n";
    std::cout << "  Runtime Manifest: " << manifest_path.string() << "\n";
    std::cout << "  Active Subsystem Replacements (" << p.mods.size() << "):\n";
    if (p.mods.empty()) {
        std::cout << "    (Standard base configuration)\n";
    } else {
        for (const auto &m : p.mods) {
            std::cout << "    • Hook [" << m.target_hook << "] => " << m.name << "\n";
        }
    }
    std::cout << "=================================================================\n";

    // 2. Resolve Slicer executable
    std::string bin_path = p.slicer_bin;
    if (!fs::exists(bin_path)) {
        // Fallback checks
        if (fs::exists(R"(modular-orca-slicer.exe)")) {
            bin_path = R"(modular-orca-slicer.exe)";
        } else if (fs::exists("orca-slicer.exe")) {
            bin_path = "orca-slicer.exe";
        } else {
            std::cerr << "Warning: Configured binary '" << bin_path << "' not found.\n";
            std::cerr << "Using system path lookup.\n";
        }
    }

    std::vector<std::string> args = {
        "--datadir", data_dir.string(),
        "--modpack", manifest_path.string()
    };
    for (const auto &extra : p.extra_args) {
        args.push_back(extra);
    }
    for (const auto &extra : extra_args) {
        args.push_back(extra);
    }

    ProcessManager proc_mgr;
    std::cout << ">>> Executing: " << bin_path << "\n";
    for (const auto &a : args) {
        std::cout << "      " << a << "\n";
    }

    auto spawn_res = proc_mgr.spawn(bin_path, args, base_dir);
    if (!spawn_res) {
        std::cerr << "Execution failed: " << spawn_res.error() << "\n";
        return 1;
    }

    auto proc = spawn_res.value();
    std::cout << ">>> Modpack instance running successfully! (PID: " << proc.pid << ")\n";
    std::cout << ">>> Orchestrator is supervising runtime lifecycle. Waiting for exit...\n";

    while (proc_mgr.is_alive(proc)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << ">>> OrcaSlicer process exited cleanly.\n";
    return 0;
}

int cmd_test_dual(const fs::path &base_dir, int duration_sec) {
    FrankensteinManager mgr(base_dir);
    ProcessManager proc_mgr;

    std::string bin = R"(modular-orca-slicer.exe)";
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

void interactive_menu(FrankensteinManager &mgr, const fs::path &base_dir) {
    while (true) {
        std::cout << "\n=================================================================\n";
        std::cout << "  FRANKENSTEIN ORCHESTRATOR - MAKER CONTROL PANEL\n";
        std::cout << "=================================================================\n";
        std::cout << "  1. List Available Community Mods (Ecosystem Catalog)\n";
        std::cout << "  2. List Configured Frankenstein Modpacks\n";
        std::cout << "  3. Inspect a Modpack (Active Mods & Hooks)\n";
        std::cout << "  4. Create a New Frankenstein Modpack\n";
        std::cout << "  5. Add Mod to a Modpack\n";
        std::cout << "  6. Remove Mod from a Modpack\n";
        std::cout << "  7. Launch Modpack in OrcaSlicer\n";
        std::cout << "  8. Run Parallel Sandbox Dual Test\n";
        std::cout << "  0. Exit\n";
        std::cout << "-----------------------------------------------------------------\n";
        std::cout << "Select option [0-8]: ";

        std::string choice;
        if (!std::getline(std::cin, choice)) break;

        if (choice == "0" || choice == "exit" || choice == "q") {
            std::cout << "Goodbye!\n";
            break;
        } else if (choice == "1") {
            cmd_mods_list(mgr);
        } else if (choice == "2") {
            cmd_modpack_list(mgr, base_dir);
        } else if (choice == "3") {
            std::cout << "Enter modpack ID: ";
            std::string id;
            std::getline(std::cin, id);
            cmd_modpack_info(mgr, base_dir, id);
        } else if (choice == "4") {
            std::cout << "Enter modpack ID (e.g. wave-custom): ";
            std::string id;
            std::getline(std::cin, id);
            std::cout << "Enter display name: ";
            std::string name;
            std::getline(std::cin, name);
            std::cout << "Enter description: ";
            std::string desc;
            std::getline(std::cin, desc);
            cmd_modpack_create(mgr, id, name, desc);
        } else if (choice == "5") {
            std::cout << "Enter target modpack ID: ";
            std::string pid;
            std::getline(std::cin, pid);
            std::cout << "Enter mod ID to add (e.g. wave_overhangs): ";
            std::string mid;
            std::getline(std::cin, mid);
            cmd_modpack_add(mgr, pid, mid);
        } else if (choice == "6") {
            std::cout << "Enter target modpack ID: ";
            std::string pid;
            std::getline(std::cin, pid);
            std::cout << "Enter mod ID to remove: ";
            std::string mid;
            std::getline(std::cin, mid);
            cmd_modpack_remove(mgr, pid, mid);
        } else if (choice == "7") {
            std::cout << "Enter modpack ID to launch: ";
            std::string pid;
            std::getline(std::cin, pid);
            cmd_launch(mgr, base_dir, pid, {});
        } else if (choice == "8") {
            cmd_test_dual(base_dir, 5);
        } else {
            std::cout << "Unknown option: " << choice << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    print_banner();

    fs::path base_dir;
    try {
        base_dir = fs::canonical(fs::path(argv[0]).parent_path());
    } catch (...) {
        base_dir = fs::current_path();
    }

    FrankensteinManager mgr(base_dir);

    if (argc < 2) {
        print_help();
        std::cout << "Starting interactive mode...\n";
        interactive_menu(mgr, base_dir);
        return 0;
    }

    std::string root_cmd = argv[1];

    if (root_cmd == "interactive" || root_cmd == "-i" || root_cmd == "--interactive") {
        interactive_menu(mgr, base_dir);
        return 0;
    }

    if (root_cmd == "mods") {
        if (argc < 3 || std::string(argv[2]) == "list") {
            cmd_mods_list(mgr);
            return 0;
        } else if (std::string(argv[2]) == "info" && argc >= 4) {
            cmd_mods_info(mgr, argv[3]);
            return 0;
        } else {
            std::cerr << "Usage: OrcaOrchestrator mods [list | info <mod_id>]\n";
            return 1;
        }
    }

    if (root_cmd == "modpack") {
        if (argc < 3 || std::string(argv[2]) == "list") {
            cmd_modpack_list(mgr, base_dir);
            return 0;
        }
        std::string sub = argv[2];
        if (sub == "info") {
            if (argc < 4) {
                std::cerr << "Usage: OrcaOrchestrator modpack info <pack_id>\n";
                return 1;
            }
            cmd_modpack_info(mgr, base_dir, argv[3]);
            return 0;
        } else if (sub == "create") {
            if (argc < 4) {
                std::cerr << "Usage: OrcaOrchestrator modpack create <id> [--name <name>] [--desc <desc>]\n";
                return 1;
            }
            std::string id = argv[3];
            std::string name = id;
            std::string desc = "";
            for (int i = 4; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg == "--name" && i + 1 < argc) {
                    name = argv[++i];
                } else if (arg == "--desc" && i + 1 < argc) {
                    desc = argv[++i];
                }
            }
            return cmd_modpack_create(mgr, id, name, desc);
        } else if (sub == "add") {
            if (argc < 5) {
                std::cerr << "Usage: OrcaOrchestrator modpack add <pack_id> <mod_id>\n";
                return 1;
            }
            return cmd_modpack_add(mgr, argv[3], argv[4]);
        } else if (sub == "remove") {
            if (argc < 5) {
                std::cerr << "Usage: OrcaOrchestrator modpack remove <pack_id> <mod_id>\n";
                return 1;
            }
            return cmd_modpack_remove(mgr, argv[3], argv[4]);
        } else {
            std::cerr << "Unknown modpack subcommand: " << sub << "\n";
            return 1;
        }
    }

    if (root_cmd == "launch") {
        if (argc < 3) {
            std::cerr << "Usage: OrcaOrchestrator launch <pack_id> [extra args...]\n";
            return 1;
        }
        std::string pack_id = argv[2];
        std::vector<std::string> extra_args;
        for (int i = 3; i < argc; ++i) {
            extra_args.push_back(argv[i]);
        }
        return cmd_launch(mgr, base_dir, pack_id, extra_args);
    }

    if (root_cmd == "test-dual") {
        int duration = 5;
        if (argc >= 4 && std::string(argv[2]) == "--duration") {
            duration = std::stoi(argv[3]);
        }
        return cmd_test_dual(base_dir, duration);
    }

    if (root_cmd == "help" || root_cmd == "-h" || root_cmd == "--help") {
        print_help();
        return 0;
    }

    std::cerr << "Unknown command: " << root_cmd << "\n";
    print_help();
    return 1;
}
