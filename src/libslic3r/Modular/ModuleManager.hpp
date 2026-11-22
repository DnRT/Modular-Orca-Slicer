#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include "ModuleABI.hpp"
#include "OverhangStrategyRegistry.hpp"
#include "Strategies/ResourceOptimizer/IResourceOptimizer.hpp"
#include "Strategies/WallInfill/IWallInfillStrategy.hpp"
#include "Strategies/AdaptiveSimplifier/Adaptive2DSimplifierMod.hpp"
#include "Strategies/LayerFlusher/EagerLayerFlusherMod.hpp"

namespace Slic3r::Modular {

namespace fs = std::filesystem;

struct LoadedModInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    SubsystemHook target_hook;
    bool is_active{false};
    std::shared_ptr<IOrcaMod> instance;
    void* native_handle{nullptr}; // HMODULE / dlopen handle
};

class ModuleManager : public ModuleContext {
public:
    static ModuleManager& instance();

    // Orchestrator Host Kernel: Automatically builds and loads modpack by inspecting presets in data_dir
    bool auto_configure_from_datadir(const fs::path &data_dir);

    // Loads a modpack manifest
    bool load_modpack(const fs::path &modpack_json_path);

    // Registers a built-in mod instance
    bool register_mod(std::shared_ptr<IOrcaMod> mod);

    // Unloads all active mods
    void unload_all();

    // Queries
    bool has_mod(const std::string &id) const;
    bool is_plugin_active(const std::string &id) const;
    void set_plugin_active(const std::string &id, bool enable);
    const std::vector<LoadedModInfo>& get_loaded_mods() const { return m_loaded_mods; }
    
    // Subsystem accessors
    std::shared_ptr<IOverhangStrategy> get_active_overhang_strategy() const;
    std::shared_ptr<IResourceOptimizer> get_resource_optimizer() const { return m_resource_optimizer; }
    bool is_resource_optimizer_active() const;

    std::shared_ptr<IWallInfillStrategy> get_wall_infill_strategy() const { return m_wall_infill_strategy; }
    bool is_wall_infill_active() const;

    std::shared_ptr<Adaptive2DSimplifierMod> get_adaptive_simplifier() const { return m_adaptive_simplifier; }
    bool is_adaptive_simplifier_active() const;

    std::shared_ptr<EagerLayerFlusherMod> get_layer_flusher() const { return m_layer_flusher; }
    bool is_layer_flusher_active() const;

    // Slicing pipeline hook dispatch
    void on_slicing_pipeline_step(int step, const void* object);

    // ModuleContext implementation
    void register_overhang_strategy(std::shared_ptr<IOverhangStrategy> strategy) override;
    void log_info(const std::string &msg) override;
    void log_warning(const std::string &msg) override;
    void log_error(const std::string &msg) override;

private:
    ModuleManager();
    ~ModuleManager();

    std::vector<LoadedModInfo> m_loaded_mods;
    std::shared_ptr<IOverhangStrategy> m_active_overhang_strategy;
    std::shared_ptr<IResourceOptimizer> m_resource_optimizer;
    std::shared_ptr<IWallInfillStrategy> m_wall_infill_strategy;
    std::shared_ptr<Adaptive2DSimplifierMod> m_adaptive_simplifier;
    std::shared_ptr<EagerLayerFlusherMod> m_layer_flusher;
};

} // namespace Slic3r::Modular
