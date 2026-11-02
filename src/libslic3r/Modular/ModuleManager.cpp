#include "ModuleManager.hpp"
#include <iostream>
#include <fstream>
#include "compat/detail/json.hpp"
#include "Strategies/ResourceOptimizer/ResourceOptimizerMod.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using json = nlohmann::json;

namespace Slic3r::Modular {

ModuleManager& ModuleManager::instance() {
    static ModuleManager s_instance;
    return s_instance;
}

ModuleManager::ModuleManager() = default;

ModuleManager::~ModuleManager() {
    unload_all();
}

bool ModuleManager::auto_configure_from_datadir(const fs::path &data_dir) {
    fs::path manifest_path = data_dir / "modpack.json";
    if (fs::exists(manifest_path)) {
        return load_modpack(manifest_path);
    }

    log_info("Orchestrator Host Kernel: Auto-discovering modules from presets in: " + data_dir.string());

    // Inspect user process presets
    fs::path user_dir = data_dir / "user";
    bool found_wave = false;
    bool found_resource_opt = false;
    std::string preset_name = "Modular Default";

    if (fs::exists(user_dir)) {
        try {
            for (const auto &entry : fs::recursive_directory_iterator(user_dir)) {
                if (entry.path().extension() == ".json" && entry.path().parent_path().filename() == "process") {
                    try {
                        std::ifstream pf(entry.path());
                        json pj;
                        pf >> pj;
                        if (pj.value("enable_wave_overhangs", "0") == "1") {
                            found_wave = true;
                            preset_name = pj.value("name", entry.path().stem().string());
                        }
                        if (pj.value("enable_resource_optimizer", "0") == "1") {
                            found_resource_opt = true;
                        }
                    } catch (...) {}
                }
            }
        } catch (...) {}
    }

    // Generate manifest
    json manifest;
    manifest["modpack_id"] = "auto_orchestrated";
    manifest["name"] = "Modular OrcaSlicer Auto Host";
    manifest["active_preset"] = preset_name;
    manifest["mods"] = json::array();

    if (found_wave) {
        json wm;
        wm["id"] = "wave_overhangs";
        wm["name"] = "Wave Overhangs (Built-in Mod)";
        wm["replaces"] = "OverhangStrategy";
        wm["enabled"] = true;
        manifest["mods"].push_back(wm);
    }

    if (found_resource_opt) {
        json rm;
        rm["id"] = "resource_optimizer_lite";
        rm["name"] = "Low-RAM Large Model Resource Optimizer";
        rm["replaces"] = "ResourceOptimizer";
        rm["enabled"] = true;
        manifest["mods"].push_back(rm);
    }

    try {
        std::ofstream out(manifest_path);
        if (out.is_open()) {
            out << manifest.dump(2);
        }
    } catch (...) {}

    return load_modpack(manifest_path);
}

bool ModuleManager::load_modpack(const fs::path &modpack_json_path) {
    if (!fs::exists(modpack_json_path)) {
        log_error("Modpack manifest not found at: " + modpack_json_path.string());
        return false;
    }

    try {
        std::ifstream f(modpack_json_path);
        json j;
        f >> j;

        std::string modpack_id = j.value("modpack_id", "custom_pack");
        std::string modpack_name = j.value("name", "Custom Frankenstein Pack");
        log_info("Orchestrator Host: Activating Modpack '" + modpack_name + "' [" + modpack_id + "]");

        if (!j.contains("mods") || !j["mods"].is_array()) {
            log_warning("Modpack contains no mods array.");
            return true;
        }

        fs::path modpack_dir = modpack_json_path.parent_path();

        for (const auto &item : j["mods"]) {
            std::string mod_id = item.value("id", "");
            bool enabled = item.value("enabled", true);
            if (!enabled || mod_id.empty()) continue;

            std::string lib_path_str = item.value("library_path", "");
            if (lib_path_str.empty()) {
                // Built-in strategies
                if (mod_id == "wave_overhangs") {
                    auto strategy = OverhangStrategyRegistry::instance().get_strategy("wave_overhangs");
                    if (strategy) {
                        register_overhang_strategy(strategy);
                        LoadedModInfo info;
                        info.id = "wave_overhangs";
                        info.name = "Wave Overhangs (Built-in Mod)";
                        info.version = "1.0.0";
                        info.author = "Janis Andersons / Dennis Klappe";
                        info.target_hook = SubsystemHook::OverhangStrategy;
                        info.is_active = true;
                        m_loaded_mods.push_back(info);
                        log_info("Orchestrator Hook Active: [OverhangStrategy] -> Wave Overhangs");
                    }
                    continue;
                } else if (mod_id == "resource_optimizer_lite") {
                    LoadedModInfo info;
                    info.id = "resource_optimizer_lite";
                    info.name = "Low-RAM Large Model Resource Optimizer";
                    info.version = "1.0.0";
                    info.author = "Modular RepRap Community";
                    info.target_hook = SubsystemHook::ResourceOptimizer;
                    info.is_active = true;
                    m_loaded_mods.push_back(info);
                    log_info("Orchestrator Hook Active: [ResourceOptimizer] -> Low-RAM Large Model Resource Optimizer");
                    continue;
                }
                continue;
            }

            fs::path full_lib_path = fs::path(lib_path_str);
            if (!full_lib_path.is_absolute()) {
                full_lib_path = modpack_dir / full_lib_path;
            }

            if (!fs::exists(full_lib_path)) {
                log_error("Mod library binary not found: " + full_lib_path.string());
                continue;
            }

            void* handle = nullptr;
            OrcaModCreateFn create_fn = nullptr;

#if defined(_WIN32)
            HMODULE hMod = LoadLibraryW(full_lib_path.wstring().c_str());
            if (!hMod) {
                log_error("Failed to LoadLibrary: " + full_lib_path.string() + " (WinError: " + std::to_string(GetLastError()) + ")");
                continue;
            }
            handle = static_cast<void*>(hMod);
            create_fn = reinterpret_cast<OrcaModCreateFn>(GetProcAddress(hMod, "create_module"));
#else
            void* dllib = dlopen(full_lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
            if (!dllib) {
                log_error("Failed dlopen: " + full_lib_path.string() + " (Error: " + dlerror() + ")");
                continue;
            }
            handle = dllib;
            create_fn = reinterpret_cast<OrcaModCreateFn>(dlsym(dllib, "create_module"));
#endif

            if (!create_fn) {
                log_error("Symbol 'create_module' not found in: " + full_lib_path.string());
#if defined(_WIN32)
                FreeLibrary(static_cast<HMODULE>(handle));
#else
                dlclose(handle);
#endif
                continue;
            }

            IOrcaMod* raw_mod = create_fn();
            if (!raw_mod) {
                log_error("create_module() returned nullptr in: " + full_lib_path.string());
                continue;
            }

            std::shared_ptr<IOrcaMod> mod_ptr(raw_mod, [handle](IOrcaMod* m) {
                if (m) m->on_unload();
                delete m;
#if defined(_WIN32)
                if (handle) FreeLibrary(static_cast<HMODULE>(handle));
#else
                if (handle) dlclose(handle);
#endif
            });

            if (mod_ptr->on_load(*this)) {
                LoadedModInfo info;
                info.id = mod_ptr->get_id();
                info.name = mod_ptr->get_name();
                info.version = mod_ptr->get_version();
                info.author = mod_ptr->get_author();
                info.target_hook = mod_ptr->get_target_hook();
                info.is_active = true;
                info.instance = mod_ptr;
                info.native_handle = handle;
                m_loaded_mods.push_back(info);
                log_info("Orchestrator Hook Active: [" + std::string(hook_to_string(info.target_hook)) + "] -> " + info.name);
            } else {
                log_warning("Mod '" + std::string(mod_ptr->get_name()) + "' on_load() returned false.");
            }
        }
        return true;
    } catch (const std::exception &e) {
        log_error("Exception parsing modpack manifest: " + std::string(e.what()));
        return false;
    }
}

bool ModuleManager::register_mod(std::shared_ptr<IOrcaMod> mod) {
    if (!mod) return false;
    if (mod->on_load(*this)) {
        LoadedModInfo info;
        info.id = mod->get_id();
        info.name = mod->get_name();
        info.version = mod->get_version();
        info.author = mod->get_author();
        info.target_hook = mod->get_target_hook();
        info.is_active = true;
        info.instance = mod;
        m_loaded_mods.push_back(info);
        return true;
    }
    return false;
}

void ModuleManager::unload_all() {
    for (auto &mod_info : m_loaded_mods) {
        if (mod_info.instance) {
            mod_info.instance->on_unload();
        }
    }
    m_loaded_mods.clear();
    m_active_overhang_strategy = nullptr;
}

bool ModuleManager::has_mod(const std::string &id) const {
    for (const auto &m : m_loaded_mods) {
        if (m.id == id && m.is_active) return true;
    }
    return false;
}

std::shared_ptr<IOverhangStrategy> ModuleManager::get_active_overhang_strategy() const {
    return m_active_overhang_strategy;
}

void ModuleManager::register_overhang_strategy(std::shared_ptr<IOverhangStrategy> strategy) {
    if (strategy) {
        m_active_overhang_strategy = strategy;
        log_info("Subsystem [OverhangStrategy] replaced by: " + strategy->name());
    }
}

void ModuleManager::log_info(const std::string &msg) {
    std::cout << "[OrchestratorCore] [INFO] " << msg << std::endl;
}

void ModuleManager::log_warning(const std::string &msg) {
    std::cout << "[OrchestratorCore] [WARN] " << msg << std::endl;
}

void ModuleManager::log_error(const std::string &msg) {
    std::cerr << "[OrchestratorCore] [ERROR] " << msg << std::endl;
}

} // namespace Slic3r::Modular
