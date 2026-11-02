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
    fs::path modular_slicer_bin;
    fs::path modular_appdata_dir;
    fs::path manifest_path;
    fs::path log_path;
    fs::path target_preset_path;
    std::string active_preset_name{"Modular Default"};
    bool wave_overhangs_enabled{false};
    bool resource_optimizer_enabled{false};
    double wave_wavelength{3.0};
    double wave_amplitude{0.8};
};

void log_msg(const fs::path &log_file, const std::string &msg) {
    std::ofstream l(log_file.string(), std::ios::app);
    if (l.is_open()) {
        l << "[ModularLauncher] " << msg << "\n";
    }
}

// Locate ONLY the newly forked Modular OrcaSlicer executable
fs::path find_modular_orcaslicer_executable(const fs::path &explicit_bin = "") {
    // 1. Explicit path
    if (!explicit_bin.empty() && fs::exists(explicit_bin)) {
        return explicit_bin;
    }

    // 2. Environment override
    wchar_t env_buf[MAX_PATH];
    DWORD env_len = GetEnvironmentVariableW(L"MODULAR_ORCA_BIN", env_buf, MAX_PATH);
    if (env_len > 0 && fs::exists(env_buf)) {
        return fs::path(env_buf);
    }

    // 3. Known build paths in OrcaSlicer-Modular
    std::vector<fs::path> candidate_paths = {
        fs::current_path() / "modular-orca-slicer.exe",
        fs::current_path() / "build" / "bin" / "modular-orca-slicer.exe",
        fs::current_path() / "build" / "bin" / "Release" / "modular-orca-slicer.exe",
        fs::current_path() / "build" / "bin" / "RelWithDebInfo" / "modular-orca-slicer.exe",
        fs::current_path() / "build" / "src" / "modular-orca-slicer.exe",
        fs::current_path() / "build" / "src" / "Release" / "modular-orca-slicer.exe",
        fs::path(R"(D:\Impresora 3D\Orca Fork\OrcaSlicer-Modular\build\bin\modular-orca-slicer.exe)"),
        fs::path(R"(D:\Impresora 3D\Orca Fork\OrcaSlicer-Modular\build\bin\Release\modular-orca-slicer.exe)"),
        fs::path(R"(D:\Impresora 3D\Orca Fork\OrcaSlicer-Modular\build\bin\RelWithDebInfo\modular-orca-slicer.exe)"),
        fs::path(R"(D:\Impresora 3D\Orca Fork\OrcaSlicer-Modular\build\bin\OrcaSlicer.exe)"),
        fs::path(R"(D:\Impresora 3D\Orca Fork\OrcaSlicer-Modular\build\bin\Release\OrcaSlicer.exe)")
    };

    for (const auto &p : candidate_paths) {
        if (fs::exists(p)) {
            return p;
        }
    }

    // Return the expected target path in the build tree
    return fs::path(R"(D:\Impresora 3D\Orca Fork\OrcaSlicer-Modular\build\bin\modular-orca-slicer.exe)");
}

// Locate %APPDATA%\ModularOrcaSlicer (ISOLATED from system OrcaSlicer)
fs::path get_modular_appdata_dir() {
    wchar_t env_buf[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"APPDATA", env_buf, MAX_PATH);
    if (len > 0) {
        return fs::path(env_buf) / "ModularOrcaSlicer";
    }
    return fs::current_path() / "modular_data";
}

// Inspect preset file and build the runtime modpack.json manifest
RuntimeConfig build_runtime_configuration(const fs::path &preset_file, const fs::path &explicit_bin = "") {
    RuntimeConfig cfg;
    cfg.modular_slicer_bin = find_modular_orcaslicer_executable(explicit_bin);
    cfg.modular_appdata_dir = get_modular_appdata_dir();
    cfg.manifest_path = cfg.modular_appdata_dir / "modpack.json";
    cfg.log_path = cfg.modular_appdata_dir / "launcher.log";
    cfg.target_preset_path = preset_file;

    fs::create_directories(cfg.modular_appdata_dir);
    fs::create_directories(cfg.modular_appdata_dir / "user" / "default" / "process");

    {
        std::ofstream l(cfg.log_path.string(), std::ios::trunc);
        l << "=== Modular OrcaSlicer Orchestrator (C++23) Startup ===\n";
    }

    log_msg(cfg.log_path, "Target Binary:  " + cfg.modular_slicer_bin.string());
    log_msg(cfg.log_path, "Isolated Data:  " + cfg.modular_appdata_dir.string());

    // 1. Inspect the designated preset file
    if (!preset_file.empty() && fs::exists(preset_file)) {
        log_msg(cfg.log_path, "Reading target preset: " + preset_file.string());
        try {
            std::ifstream f(preset_file.string());
            json pj;
            f >> pj;

            cfg.active_preset_name = pj.value("name", preset_file.stem().string());

            if (pj.value("enable_wave_overhangs", "0") == "1" ||
                pj.value("overhang_strategy", "") == "wave_overhangs") {
                cfg.wave_overhangs_enabled = true;
                if (pj.contains("wave_wavelength")) {
                    try { cfg.wave_wavelength = std::stod(pj["wave_wavelength"].get<std::string>()); } catch (...) {}
                }
                if (pj.contains("wave_amplitude")) {
                    try { cfg.wave_amplitude = std::stod(pj["wave_amplitude"].get<std::string>()); } catch (...) {}
                }
                log_msg(cfg.log_path, "Preset activates [Wave Overhangs] mod (OSW)");
            }

            if (pj.value("enable_resource_optimizer", "0") == "1") {
                cfg.resource_optimizer_enabled = true;
                log_msg(cfg.log_path, "Preset activates [Resource Optimizer] mod");
            }

            // Copy preset to ModularOrcaSlicer process directory
            fs::path dest = cfg.modular_appdata_dir / "user" / "default" / "process" / preset_file.filename();
            fs::copy_file(preset_file, dest, fs::copy_options::overwrite_existing);
        } catch (const std::exception &e) {
            log_msg(cfg.log_path, "Error reading preset: " + std::string(e.what()));
        }
    } else {
        log_msg(cfg.log_path, "No explicit preset provided. Scanning user process presets in ModularOrcaSlicer...");
        fs::path proc_dir = cfg.modular_appdata_dir / "user" / "default" / "process";
        if (fs::exists(proc_dir)) {
            for (const auto &entry : fs::directory_iterator(proc_dir)) {
                if (entry.path().extension() == ".json") {
                    try {
                        std::ifstream f(entry.path().string());
                        json pj;
                        f >> pj;
                        if (pj.value("enable_wave_overhangs", "0") == "1") {
                            cfg.wave_overhangs_enabled = true;
                            cfg.active_preset_name = pj.value("name", entry.path().stem().string());
                            log_msg(cfg.log_path, "Found Wave Overhangs enabled in: " + entry.path().filename().string());
                        }
                    } catch (...) {}
                }
            }
        }
    }

    // 2. Build modpack.json manifest
    json manifest;
    manifest["modpack_id"] = "modular_runtime_pack";
    manifest["name"] = "Modular OrcaSlicer Runtime Pack";
    manifest["active_preset"] = cfg.active_preset_name;
    manifest["generated_by"] = "Modular Orchestrator (C++23)";
    manifest["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    manifest["mods"] = json::array();

    if (cfg.wave_overhangs_enabled) {
        json wm;
        wm["id"] = "wave_overhangs";
        wm["name"] = "Wave Overhangs (Janis Andersons / Dennis Klappe)";
        wm["replaces"] = "OverhangStrategy";
        wm["enabled"] = true;
        wm["parameters"] = {
            {"wavelength", cfg.wave_wavelength},
            {"amplitude", cfg.wave_amplitude}
        };
        manifest["mods"].push_back(wm);
        log_msg(cfg.log_path, "Registered module: wave_overhangs -> Hook [OverhangStrategy]");
    }

    if (cfg.resource_optimizer_enabled) {
        json rm;
        rm["id"] = "resource_optimizer_lite";
        rm["name"] = "Low-RAM Large Model Resource Optimizer";
        rm["replaces"] = "ResourceOptimizer";
        rm["enabled"] = true;
        manifest["mods"].push_back(rm);
        log_msg(cfg.log_path, "Registered module: resource_optimizer_lite -> Hook [ResourceOptimizer]");
    }

    try {
        std::ofstream out(cfg.manifest_path.string());
        if (out.is_open()) {
            out << manifest.dump(2);
            log_msg(cfg.log_path, "Successfully wrote modpack.json to: " + cfg.manifest_path.string());
        }
    } catch (...) {}

    return cfg;
}

// Spawns Modular OrcaSlicer cleanly
bool spawn_modular_slicer(const RuntimeConfig &cfg, const std::vector<std::wstring> &forwarded_args) {
    if (!fs::exists(cfg.modular_slicer_bin)) {
        std::wstring err = L"No se encontro el ejecutable de Modular OrcaSlicer en:\n" +
                           cfg.modular_slicer_bin.wstring() +
                           L"\n\nPor favor compila el fork en 'OrcaSlicer-Modular\\build' (Target: modular-orca-slicer).\n" +
                           L"El orquestador NO ejecutara el OrcaSlicer oficial del sistema para evitar colisiones.";
        log_msg(cfg.log_path, "ERROR: Target binary does not exist.");
        MessageBoxW(nullptr, err.c_str(), L"Modular OrcaSlicer Orchestrator", MB_ICONWARNING | MB_OK);
        return false;
    }

    std::wstring cmdline = L"\"" + cfg.modular_slicer_bin.wstring() + L"\""
                         + L" --datadir \"" + cfg.modular_appdata_dir.wstring() + L"\""
                         + L" --modpack \"" + cfg.manifest_path.wstring() + L"\"";

    for (const auto &arg : forwarded_args) {
        cmdline += L" \"" + arg + L"\"";
    }

    log_msg(cfg.log_path, "Spawning: " + std::string(cmdline.begin(), cmdline.end()));

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    std::vector<wchar_t> cmd_buf(cmdline.begin(), cmdline.end());
    cmd_buf.push_back(L'\0');

    BOOL ok = CreateProcessW(
        cfg.modular_slicer_bin.wstring().c_str(),
        cmd_buf.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        cfg.modular_slicer_bin.parent_path().wstring().c_str(),
        &si,
        &pi
    );

    if (ok) {
        log_msg(cfg.log_path, "Modular OrcaSlicer spawned successfully! PID: " + std::to_string(pi.dwProcessId));
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    } else {
        DWORD err = GetLastError();
        log_msg(cfg.log_path, "CreateProcess failed with code: " + std::to_string(err));
        return false;
    }
}

} // namespace orca::launcher

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    fs::path target_preset;
    fs::path explicit_bin;
    std::vector<std::wstring> forwarded_args;

    for (int i = 1; i < argc; ++i) {
        std::wstring warg = argv[i];
        if (warg == L"--preset" && i + 1 < argc) {
            target_preset = argv[++i];
        } else if (warg == L"--bin" && i + 1 < argc) {
            explicit_bin = argv[++i];
        } else {
            forwarded_args.push_back(warg);
        }
    }
    if (argv) LocalFree(argv);

    // 1. Build configuration for Modular OrcaSlicer
    auto cfg = orca::launcher::build_runtime_configuration(target_preset, explicit_bin);

    // 2. Launch Modular OrcaSlicer
    bool ok = orca::launcher::spawn_modular_slicer(cfg, forwarded_args);
    return ok ? 0 : 1;
}

int main(int argc, char* argv[]) {
    return WinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOW);
}
