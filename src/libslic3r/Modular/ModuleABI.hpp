#pragma once

#include <string>
#include <memory>
#include <vector>
#include "IOverhangStrategy.hpp"

namespace Slic3r::Modular {

// Extension points in OrcaSlicer where community mods can hook in or replace base systems
enum class SubsystemHook {
    OverhangStrategy,      // Replaces perimeter/bridge overhang generator (e.g. Wave Overhangs)
    InfillPattern,         // Replaces or adds infill patterns (e.g. Custom Hilbert, Gyroid 2.0)
    KinematicsEngine,      // Modifies toolpath kinematics (e.g. Non-Planar 3-Axis continuous Z)
    GCodePostProcessor,    // Modifies emitted G-code stream before writing to file
    CustomUIWidget,        // Adds UI parameters/panels dynamically
    ResourceOptimizer      // Optimizes memory & thread scheduling for large models on modest PCs
};

inline const char* hook_to_string(SubsystemHook hook) {
    switch (hook) {
        case SubsystemHook::OverhangStrategy:   return "OverhangStrategy";
        case SubsystemHook::InfillPattern:      return "InfillPattern";
        case SubsystemHook::KinematicsEngine:   return "KinematicsEngine";
        case SubsystemHook::GCodePostProcessor: return "GCodePostProcessor";
        case SubsystemHook::CustomUIWidget:     return "CustomUIWidget";
        case SubsystemHook::ResourceOptimizer:  return "ResourceOptimizer";
        default: return "Unknown";
    }
}

// Context provided to a mod when loaded by the engine
class ModuleContext {
public:
    virtual ~ModuleContext() = default;

    // Replace the default overhang strategy with a community strategy
    virtual void register_overhang_strategy(std::shared_ptr<IOverhangStrategy> strategy) = 0;

    // Log messages to OrcaSlicer's log system
    virtual void log_info(const std::string &msg) = 0;
    virtual void log_warning(const std::string &msg) = 0;
    virtual void log_error(const std::string &msg) = 0;
};

// Base interface that every community Mod/Plugin must implement
class IOrcaMod {
public:
    virtual ~IOrcaMod() = default;

    virtual const char* get_id() const = 0;
    virtual const char* get_name() const = 0;
    virtual const char* get_version() const = 0;
    virtual const char* get_author() const = 0;
    virtual const char* get_description() const = 0;
    virtual SubsystemHook get_target_hook() const = 0;

    // Called when OrcaSlicer initializes the modpack
    virtual bool on_load(ModuleContext &ctx) = 0;

    // Called when OrcaSlicer unloads the modpack
    virtual void on_unload() = 0;
};

} // namespace Slic3r::Modular

// Entry point signature for dynamically loaded shared library mods (.dll / .so)
extern "C" {
    typedef Slic3r::Modular::IOrcaMod* (*OrcaModCreateFn)();
    typedef void (*OrcaModDestroyFn)(Slic3r::Modular::IOrcaMod*);
}
