#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

#include "compat/detail/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace orca::launcher {

struct RuntimeConfig {
    fs::path orca_exe_path;
    fs::path appdata_orcaslicer;
    fs::path manifest_path;
    fs::path log_path;
    std::string active_preset_name;
    bool wave_overhangs_enabled{false};
    double wave_wavelength{3.0};
    double wave_amplitude{0.8};
};

void log_msg(const fs::path &log_file, const std::string &msg) {
    std::ofstream l(log_file.string(), std::ios::app);
    if (l.is_open()) {
        l << "[Launcher] " << msg << "\n";
    }
}

// Locate OrcaSlicer installed executable
fs::path find_orcaslicer_executable() {
    wchar_t env_buf[MAX_PATH];
    DWORD env_len = GetEnvironmentVariableW(L"ORCASLICER_BIN", env_buf, MAX_PATH);
    if (env_len > 0 && fs::exists(env_buf)) {
        return fs::path(env_buf);
    }

    fs::path std_path = R"(C:\Program Files\OrcaSlicer\orca-slicer.exe)";
    if (fs::exists(std_path)) {
        return std_path;
    }

    fs::path local_path = fs::current_path() / "orca-slicer.exe";
    if (fs::exists(local_path)) {
        return local_path;
    }

    return std_path;
}

// Locate OrcaSlicer %APPDATA% directory using Win32 API
fs::path find_orcaslicer_appdata() {
    wchar_t env_buf[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"APPDATA", env_buf, MAX_PATH);
    if (len > 0) {
        fs::path p = fs::path(env_buf) / "OrcaSlicer";
        if (fs::exists(p)) {
            return p;
        }
    }

    wchar_t folder_buf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, folder_buf))) {
        fs::path p = fs::path(folder_buf) / "OrcaSlicer";
        if (fs::exists(p)) {
            return p;
        }
    }

    return fs::path(R"(C:\Users\Kn_CT\AppData\Roaming\OrcaSlicer)");
}

// Build runtime configuration by inspecting presets
RuntimeConfig build_runtime_configuration(const fs::path &appdata_dir, const fs::path &orca_exe) {
    RuntimeConfig cfg;
    cfg.orca_exe_path = orca_exe;
    cfg.appdata_orcaslicer = appdata_dir;
    cfg.manifest_path = appdata_dir / "modpack.json";
    cfg.log_path = appdata_dir / "launcher.log";

    fs::create_directories(appdata_dir);

    // Reset log file
    {
        std::ofstream l(cfg.log_path.string(), std::ios::trunc);
        l << "=== Modular OrcaSlicer Launcher (C++23) Startup ===\n";
    }

    log_msg(cfg.log_path, "AppData Dir: " + cfg.appdata_orcaslicer.string());
    log_msg(cfg.log_path, "Slicer Exe:  " + cfg.orca_exe_path.string());

    fs::path conf_file = appdata_dir / "OrcaSlicer.conf";
    std::string preset_folder = "default";

    // 1. Scan OrcaSlicer.conf for active preset folder
    if (fs::exists(conf_file)) {
        try {
            std::ifstream f(conf_file.string());
            std::string line;
            while (std::getline(f, line)) {
                auto pos = line.find("\"preset_folder\":");
                if (pos != std::string::npos) {
                    auto start = line.find("\"", pos + 16);
                    auto end = line.find("\"", start + 1);
                    if (start != std::string::npos && end != std::string::npos) {
                        preset_folder = line.substr(start + 1, end - start - 1);
                    }
                }
                auto pos_proc = line.find("\"process\":");
                if (pos_proc != std::string::npos) {
                    auto start = line.find("\"", pos_proc + 10);
                    auto end = line.find("\"", start + 1);
                    if (start != std::string::npos && end != std::string::npos) {
                        cfg.active_preset_name = line.substr(start + 1, end - start - 1);
                    }
                }
            }
            log_msg(cfg.log_path, "Detected Preset Folder: " + preset_folder);
            log_msg(cfg.log_path, "Detected Active Preset: " + cfg.active_preset_name);
        } catch (const std::exception &e) {
            log_msg(cfg.log_path, "Error parsing OrcaSlicer.conf: " + std::string(e.what()));
        }
    }

    // 2. Inspect user process presets to see if any community plugin is activated
    fs::path user_process_dir = appdata_dir / "user" / preset_folder / "process";
    if (!fs::exists(user_process_dir)) {
        fs::path users_dir = appdata_dir / "user";
        if (fs::exists(users_dir)) {
            for (const auto &entry : fs::directory_iterator(users_dir)) {
                if (entry.is_directory()) {
                    fs::path candidate = entry.path() / "process";
                    if (fs::exists(candidate)) {
                        user_process_dir = candidate;
                        break;
                    }
                }
            }
        }
    }

    log_msg(cfg.log_path, "Scanning Process Presets in: " + user_process_dir.string());

    if (fs::exists(user_process_dir)) {
        for (const auto &entry : fs::directory_iterator(user_process_dir)) {
            if (entry.path().extension() == ".json") {
                try {
                    std::ifstream f(entry.path().string());
                    json j;
                    f >> j;

                    std::string en_wave = j.value("enable_wave_overhangs", "0");
                    std::string strat = j.value("overhang_strategy", "");
                    if (en_wave == "1" || strat == "wave_overhangs") {
                        cfg.wave_overhangs_enabled = true;
                        if (j.contains("wave_wavelength")) {
                            try { cfg.wave_wavelength = std::stod(j["wave_wavelength"].get<std::string>()); } catch (...) {}
                        }
                        if (j.contains("wave_amplitude")) {
                            try { cfg.wave_amplitude = std::stod(j["wave_amplitude"].get<std::string>()); } catch (...) {}
                        }
                        log_msg(cfg.log_path, "Found Wave Overhangs enabled in preset: " + entry.path().filename().string());
                    }
                } catch (...) {}
            }
        }
    }

    // 3. Generate runtime modpack.json manifest in AppData
    json manifest;
    manifest["generated_by"] = "Modular OrcaSlicer Launcher (C++23)";
    manifest["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    manifest["active_preset"] = cfg.active_preset_name;
    manifest["mods"] = json::array();

    if (cfg.wave_overhangs_enabled) {
        json wave_mod;
        wave_mod["id"] = "wave_overhangs";
        wave_mod["name"] = "Wave Overhangs (Janis Andersons / Dennis Klappe)";
        wave_mod["replaces"] = "OverhangStrategy";
        wave_mod["enabled"] = true;
        wave_mod["parameters"] = {
            {"wavelength", cfg.wave_wavelength},
            {"amplitude", cfg.wave_amplitude}
        };
        manifest["mods"].push_back(wave_mod);
        log_msg(cfg.log_path, "Mod [wave_overhangs] activated in manifest");
    } else {
        log_msg(cfg.log_path, "No active mods found in current preset. Running stock subsystems.");
    }

    // Check for custom community plugins directory
    fs::path community_plugins_dir = appdata_dir / "plugins" / "community";
    if (fs::exists(community_plugins_dir)) {
        for (const auto &entry : fs::directory_iterator(community_plugins_dir)) {
            if (entry.path().extension() == ".json") {
                try {
                    std::ifstream f(entry.path().string());
                    json plug_json;
                    f >> plug_json;
                    manifest["mods"].push_back(plug_json);
                    log_msg(cfg.log_path, "Registered community plugin: " + entry.path().filename().string());
                } catch (...) {}
            }
        }
    }

    // Save manifest to disk
    try {
        std::ofstream out(cfg.manifest_path.string());
        if (out.is_open()) {
            out << manifest.dump(2);
            log_msg(cfg.log_path, "Successfully wrote manifest: " + cfg.manifest_path.string());
        } else {
            log_msg(cfg.log_path, "FAILED to open manifest for writing: " + cfg.manifest_path.string());
        }
    } catch (const std::exception &e) {
        log_msg(cfg.log_path, "Exception writing manifest: " + std::string(e.what()));
    }

    return cfg;
}

// Spawns OrcaSlicer seamlessly
bool launch_orcaslicer(const RuntimeConfig &cfg, const std::vector<std::wstring> &forwarded_args) {
    if (!fs::exists(cfg.orca_exe_path)) {
        log_msg(cfg.log_path, "Executable not found: " + cfg.orca_exe_path.string());
        MessageBoxW(
            nullptr,
            (L"No se encontro el ejecutable de OrcaSlicer en:\n" + cfg.orca_exe_path.wstring() +
             L"\n\nPor favor verifica la instalacion de OrcaSlicer.").c_str(),
            L"Modular OrcaSlicer Launcher",
            MB_ICONERROR | MB_OK
        );
        return false;
    }

    // Export ORCA_MODPACK environment variable so OrcaSlicer knows active mods
    SetEnvironmentVariableW(L"ORCA_MODPACK", cfg.manifest_path.wstring().c_str());

    std::wstring cmdline = L"\"" + cfg.orca_exe_path.wstring() + L"\"";

    for (const auto &arg : forwarded_args) {
        cmdline += L" \"" + arg + L"\"";
    }

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    std::vector<wchar_t> cmd_buf(cmdline.begin(), cmdline.end());
    cmd_buf.push_back(L'\0');

    log_msg(cfg.log_path, "Executing: " + std::string(cmdline.begin(), cmdline.end()));

    BOOL ok = CreateProcessW(
        cfg.orca_exe_path.wstring().c_str(),
        cmd_buf.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        cfg.orca_exe_path.parent_path().wstring().c_str(),
        &si,
        &pi
    );

    if (ok) {
        log_msg(cfg.log_path, "Process spawned successfully. PID: " + std::to_string(pi.dwProcessId));
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    } else {
        DWORD err = GetLastError();
        log_msg(cfg.log_path, "CreateProcess failed with WinError: " + std::to_string(err));
        MessageBoxW(
            nullptr,
            (L"Error al iniciar OrcaSlicer. Codigo de error Windows: " + std::to_wstring(err)).c_str(),
            L"Modular OrcaSlicer Launcher",
            MB_ICONERROR | MB_OK
        );
        return false;
    }
}

} // namespace orca::launcher

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::vector<std::wstring> forwarded_args;
    if (argv && argc > 1) {
        for (int i = 1; i < argc; ++i) {
            forwarded_args.push_back(argv[i]);
        }
    }
    if (argv) LocalFree(argv);

    fs::path orca_exe = orca::launcher::find_orcaslicer_executable();
    fs::path appdata_dir = orca::launcher::find_orcaslicer_appdata();

    // 1. Build configuration from user presets & plugins
    auto cfg = orca::launcher::build_runtime_configuration(appdata_dir, orca_exe);

    // 2. Launch OrcaSlicer directly
    bool success = orca::launcher::launch_orcaslicer(cfg, forwarded_args);
    return success ? 0 : 1;
}

int main(int argc, char* argv[]) {
    return WinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOW);
}
