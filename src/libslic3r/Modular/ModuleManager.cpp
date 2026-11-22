#include "nlohmann/json.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Layer.hpp"
#include "ModuleManager.hpp"
#include <iostream>
#include <fstream>
#include "Strategies/ResourceOptimizer/ResourceOptimizerMod.hpp"
#include "Strategies/ArcOverhangs/ArcOverhangStrategy.hpp"
#include "Strategies/WallInfill/WallInfillMod.hpp"
#include "libslic3r/Utils.hpp"

using json = nlohmann::json;

namespace Slic3r::Modular {

ModuleManager& ModuleManager::instance() {
    static ModuleManager s_instance;
    return s_instance;
}

ModuleManager::ModuleManager() {
    // Instantiate built-in module singletons
    auto res_opt = std::make_shared<ResourceOptimizerMod>();
    m_resource_optimizer = res_opt;

    auto wall_infill = std::make_shared<WallInfillMod>();
    m_wall_infill_strategy = wall_infill;

    m_adaptive_simplifier = std::make_shared<Adaptive2DSimplifierMod>();
    m_layer_flusher = std::make_shared<EagerLayerFlusherMod>();
}

ModuleManager::~ModuleManager() {
    unload_all();
}

bool ModuleManager::auto_configure_from_datadir(const fs::path &data_dir) {
    fs::path target_dir = data_dir;
    if (target_dir.empty()) {
        try { target_dir = Slic3r::data_dir(); } catch (...) {}
    }
    if (target_dir.empty()) {
#ifdef _WIN32
        const char *appdata = getenv("APPDATA");
        if (appdata) target_dir = fs::path(appdata) / "ModularOrcaSlicer";
#endif
    }
    if (target_dir.empty()) return false;

    fs::path manifest_path = target_dir / "modpack.json";
    if (fs::exists(manifest_path)) {
        return load_modpack(manifest_path);
    }

    log_info("Orchestrator Host Kernel: Auto-discovering modules from presets in: " + target_dir.string());

    // Inspect user process presets
    fs::path user_dir = target_dir / "user";
    bool found_wave = false;
    bool found_arc = false;
    bool found_resource_opt = false;
    bool found_wall_infill = false;
    std::string preset_name = "Modular Default";

    if (fs::exists(user_dir)) {
        try {
            for (const auto &entry : fs::recursive_directory_iterator(user_dir)) {
                if (entry.path().extension() == ".json" && entry.path().parent_path().filename() == "process") {
                    try {
                        std::ifstream pf(entry.path());
                        json pj;
                        pf >> pj;
                        if (pj.value("enable_wave_overhangs", "0") == "1" || pj.value("wave_overhangs", "0") == "1") {
                            found_wave = true;
                            preset_name = pj.value("name", entry.path().stem().string());
                        }
                        if (pj.value("wave_overhang_algorithm", "") == "arc") {
                            found_arc = true;
                        }
                        if (pj.value("enable_resource_optimizer", "0") == "1") {
                            found_resource_opt = true;
                        }
                        if (pj.value("modular_infill_walls", "0") == "1") {
                            found_wall_infill = true;
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

    if (found_arc) {
        json am;
        am["id"] = "arc_overhangs";
        am["name"] = "Arc Overhangs (Concentric Arcs)";
        am["replaces"] = "OverhangStrategy";
        am["enabled"] = true;
        manifest["mods"].push_back(am);
    }

    if (found_resource_opt) {
        json rm;
        rm["id"] = "resource_optimizer_lite";
        rm["name"] = "Low-RAM Large Model Resource Optimizer";
        rm["replaces"] = "ResourceOptimizer";
        rm["enabled"] = true;
        manifest["mods"].push_back(rm);
    }

    if (found_wall_infill) {
        json wi;
        wi["id"] = "modular_infill_walls";
        wi["name"] = "Algorithmic Walls & Infill Enhancer";
        wi["replaces"] = "InfillPattern";
        wi["enabled"] = true;
        manifest["mods"].push_back(wi);
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

        m_loaded_mods.clear();

        for (const auto &item : j["mods"]) {
            std::string mod_id = item.value("id", "");
            bool enabled = item.value("enabled", true);
            if (!enabled || mod_id.empty()) continue;

            // Secure Modular Engine: dispatch directly to verified compiled-in subsystem strategies
            if (mod_id == "wave_overhangs") {
                auto strategy = OverhangStrategyRegistry::instance().get_strategy("wave_overhangs");
                if (strategy) {
                    register_overhang_strategy(strategy);
                    LoadedModInfo info;
                    info.id = "wave_overhangs";
                    info.name = "Wave Overhangs (Built-in Mod)";
                    info.version = "1.2.0";
                    info.author = "Janis Andersons / Dennis Klappe";
                    info.target_hook = SubsystemHook::OverhangStrategy;
                    info.is_active = true;
                    m_loaded_mods.push_back(info);
                    log_info("Orchestrator Hook Active: [OverhangStrategy] -> Wave Overhangs");
                }
            } else if (mod_id == "arc_overhangs") {
                auto strategy = OverhangStrategyRegistry::instance().get_strategy("arc_overhangs");
                if (strategy) {
                    register_overhang_strategy(strategy);
                    LoadedModInfo info;
                    info.id = "arc_overhangs";
                    info.name = "Arc Overhangs (Concentric Arcs)";
                    info.version = "2.1.0";
                    info.author = "Steven McCulloch";
                    info.target_hook = SubsystemHook::OverhangStrategy;
                    info.is_active = true;
                    m_loaded_mods.push_back(info);
                    log_info("Orchestrator Hook Active: [OverhangStrategy] -> Arc Overhangs");
                }
            } else if (mod_id == "resource_optimizer_lite") {
                if (m_resource_optimizer) {
                    auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_resource_optimizer);
                    if (mod) mod->on_load(*this);
                }
                LoadedModInfo info;
                info.id = "resource_optimizer_lite";
                info.name = "Low-RAM Large Model Resource Optimizer";
                info.version = "1.1.0";
                info.author = "Modular RepRap Community";
                info.target_hook = SubsystemHook::ResourceOptimizer;
                info.is_active = true;
                m_loaded_mods.push_back(info);
                log_info("Orchestrator Hook Active: [ResourceOptimizer] -> Low-RAM Large Model Resource Optimizer");
            } else if (mod_id == "modular_infill_walls") {
                if (m_wall_infill_strategy) {
                    auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_wall_infill_strategy);
                    if (mod) mod->on_load(*this);
                }
                LoadedModInfo info;
                info.id = "modular_infill_walls";
                info.name = "Algorithmic Walls & Infill Enhancer";
                info.version = "1.0.0";
                info.author = "Modular RepRap Community";
                info.target_hook = SubsystemHook::InfillPattern;
                info.is_active = true;
                m_loaded_mods.push_back(info);
                log_info("Orchestrator Hook Active: [InfillPattern] -> Algorithmic Walls & Infill Enhancer");
            } else if (mod_id == "adaptive_2d_simplifier") {
                if (m_adaptive_simplifier) {
                    auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_adaptive_simplifier);
                    if (mod) mod->on_load(*this);
                }
                LoadedModInfo info;
                info.id = "adaptive_2d_simplifier";
                info.name = "Adaptive 2D Contour Simplifier";
                info.version = "1.0.0";
                info.author = "Modular RepRap Community";
                info.target_hook = SubsystemHook::ResourceOptimizer;
                info.is_active = true;
                m_loaded_mods.push_back(info);
                log_info("Orchestrator Hook Active: [ResourceOptimizer] -> Adaptive 2D Contour Simplifier");
            } else if (mod_id == "eager_layer_flusher") {
                if (m_layer_flusher) {
                    auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_layer_flusher);
                    if (mod) mod->on_load(*this);
                }
                LoadedModInfo info;
                info.id = "eager_layer_flusher";
                info.name = "Eager Layer Buffer Flusher";
                info.version = "1.0.0";
                info.author = "Modular RepRap Community";
                info.target_hook = SubsystemHook::ResourceOptimizer;
                info.is_active = true;
                m_loaded_mods.push_back(info);
                log_info("Orchestrator Hook Active: [ResourceOptimizer] -> Eager Layer Buffer Flusher");
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
    if (m_resource_optimizer) {
        auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_resource_optimizer);
        if (mod) mod->on_unload();
    }
    if (m_wall_infill_strategy) {
        auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_wall_infill_strategy);
        if (mod) mod->on_unload();
    }
}

bool ModuleManager::is_plugin_active(const std::string &id) const {
    for (const auto &m : m_loaded_mods) {
        if (m.id == id || (id == "wave_overhangs" && m.name.find("Wave") != std::string::npos)) {
            return m.is_active;
        }
    }

    fs::path target_dir;
    try {
        target_dir = Slic3r::data_dir();
    } catch (...) {}
    if (target_dir.empty()) {
#ifdef _WIN32
        const char *appdata = getenv("APPDATA");
        if (appdata) target_dir = fs::path(appdata) / "ModularOrcaSlicer";
#endif
    }

    if (!target_dir.empty()) {
        fs::path manifest_path = target_dir / "modpack.json";
        if (fs::exists(manifest_path)) {
            try {
                std::ifstream in(manifest_path.string());
                nlohmann::json j;
                in >> j;
                if (j.contains("mods") && j["mods"].is_array()) {
                    for (const auto &m : j["mods"]) {
                        if (m.value("id", "") == id) {
                            return m.value("enabled", false);
                        }
                    }
                }
            } catch (...) {}
        }
    }

    return false;
}

void ModuleManager::set_plugin_active(const std::string &id, bool enable) {
    bool found = false;
    for (auto &m : m_loaded_mods) {
        if (m.id == id || (id == "wave_overhangs" && m.name.find("Wave") != std::string::npos)) {
            m.is_active = enable;
            found = true;
            break;
        }
    }

    if (!found && enable) {
        if (id == "wave_overhangs") {
            auto strategy = OverhangStrategyRegistry::instance().get_strategy("wave_overhangs");
            if (strategy) {
                register_overhang_strategy(strategy);
                LoadedModInfo info;
                info.id = "wave_overhangs";
                info.name = "Wave Overhangs (Built-in Mod)";
                info.version = "1.2.0";
                info.author = "Janis Andersons / Dennis Klappe";
                info.target_hook = SubsystemHook::OverhangStrategy;
                info.is_active = true;
                m_loaded_mods.push_back(info);
            }
        } else if (id == "arc_overhangs") {
            auto strategy = OverhangStrategyRegistry::instance().get_strategy("arc_overhangs");
            if (strategy) {
                register_overhang_strategy(strategy);
                LoadedModInfo info;
                info.id = "arc_overhangs";
                info.name = "Arc Overhangs (Concentric Arcs)";
                info.version = "2.1.0";
                info.author = "Steven McCulloch";
                info.target_hook = SubsystemHook::OverhangStrategy;
                info.is_active = true;
                m_loaded_mods.push_back(info);
            }
        } else if (id == "resource_optimizer_lite") {
            if (m_resource_optimizer) {
                auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_resource_optimizer);
                if (mod) mod->on_load(*this);
            }
            LoadedModInfo info;
            info.id = "resource_optimizer_lite";
            info.name = "Low-RAM Large Model Resource Optimizer";
            info.version = "1.1.0";
            info.author = "Modular RepRap Community";
            info.target_hook = SubsystemHook::ResourceOptimizer;
            info.is_active = true;
            m_loaded_mods.push_back(info);
        } else if (id == "modular_infill_walls") {
            if (m_wall_infill_strategy) {
                auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_wall_infill_strategy);
                if (mod) mod->on_load(*this);
            }
            LoadedModInfo info;
            info.id = "modular_infill_walls";
            info.name = "Algorithmic Walls & Infill Enhancer";
            info.version = "1.0.0";
            info.author = "Modular RepRap Community";
            info.target_hook = SubsystemHook::InfillPattern;
            info.is_active = true;
            m_loaded_mods.push_back(info);
        } else if (id == "adaptive_2d_simplifier") {
            if (m_adaptive_simplifier) {
                auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_adaptive_simplifier);
                if (mod) mod->on_load(*this);
            }
            LoadedModInfo info;
            info.id = "adaptive_2d_simplifier";
            info.name = "Adaptive 2D Contour Simplifier";
            info.version = "1.0.0";
            info.author = "Modular RepRap Community";
            info.target_hook = SubsystemHook::ResourceOptimizer;
            info.is_active = true;
            m_loaded_mods.push_back(info);
        } else if (id == "eager_layer_flusher") {
            if (m_layer_flusher) {
                auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_layer_flusher);
                if (mod) mod->on_load(*this);
            }
            LoadedModInfo info;
            info.id = "eager_layer_flusher";
            info.name = "Eager Layer Buffer Flusher";
            info.version = "1.0.0";
            info.author = "Modular RepRap Community";
            info.target_hook = SubsystemHook::ResourceOptimizer;
            info.is_active = true;
            m_loaded_mods.push_back(info);
        }
    }

    if (id == "wave_overhangs" && !enable) {
        m_active_overhang_strategy = nullptr;
    }
    if (id == "resource_optimizer_lite") {
        if (m_resource_optimizer) {
            auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_resource_optimizer);
            if (mod) {
                if (enable) mod->on_load(*this);
                else mod->on_unload();
            }
        }
    }
    if (id == "modular_infill_walls") {
        if (m_wall_infill_strategy) {
            auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_wall_infill_strategy);
            if (mod) {
                if (enable) mod->on_load(*this);
                else mod->on_unload();
            }
        }
    }
    if (id == "adaptive_2d_simplifier") {
        if (m_adaptive_simplifier) {
            auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_adaptive_simplifier);
            if (mod) {
                if (enable) mod->on_load(*this);
                else mod->on_unload();
            }
        }
    }
    if (id == "eager_layer_flusher") {
        if (m_layer_flusher) {
            auto mod = std::dynamic_pointer_cast<IOrcaMod>(m_layer_flusher);
            if (mod) {
                if (enable) mod->on_load(*this);
                else mod->on_unload();
            }
        }
    }
}

bool ModuleManager::has_mod(const std::string &id) const {
    return is_plugin_active(id);
}

bool ModuleManager::is_resource_optimizer_active() const {
    return is_plugin_active("resource_optimizer_lite") && m_resource_optimizer && m_resource_optimizer->is_active();
}

bool ModuleManager::is_wall_infill_active() const {
    return is_plugin_active("modular_infill_walls");
}

bool ModuleManager::is_adaptive_simplifier_active() const {
    return is_plugin_active("adaptive_2d_simplifier");
}

bool ModuleManager::is_layer_flusher_active() const {
    return is_plugin_active("eager_layer_flusher");
}

void ModuleManager::on_slicing_pipeline_step(int step, const void* object) {
    // Secondary hooks execution in slicing pipeline
    if (is_resource_optimizer_active()) {
        // step 0: posSlice -> prompt memory trim
        if (step == 0) {
            std::cout << "[ResourceOptimizer] posSlice complete: Releasing pre-slice mesh cache" << std::endl;
            m_resource_optimizer->on_slicing_completed();
        }
    }
    if (is_wall_infill_active()) {
        // step 3: posPrepareInfill
        if (step == 3) {
            // Infill travel path damping active
        }
    }
    // Eager layer buffer flushing at step 4 (posInfill)
    if (is_layer_flusher_active() && object && m_layer_flusher) {
        if (step == 4) { // posInfill
            auto *obj = static_cast<PrintObject*>(const_cast<void*>(object));
            std::cout << "[EagerLayerFlusher] posInfill complete: Flushing intermediate buffers across " 
                      << obj->layers().size() << " layers" << std::endl;
            for (Layer *layer : obj->layers()) {
                m_layer_flusher->flush_layer_buffers(layer);
            }
        }
    }
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
