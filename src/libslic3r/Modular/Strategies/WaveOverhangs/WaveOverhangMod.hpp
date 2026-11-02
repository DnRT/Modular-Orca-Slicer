#pragma once

#include "../../ModuleABI.hpp"
#include "WaveOverhangStrategy.hpp"

namespace Slic3r::Modular {

class WaveOverhangMod : public IOrcaMod {
public:
    const char* get_id() const override { return "wave_overhangs"; }
    const char* get_name() const override { return "Wave Overhangs (Andersons/Klappe)"; }
    const char* get_version() const override { return "1.0.0"; }
    const char* get_author() const override { return "Janis Andersons / Dennis Klappe"; }
    const char* get_description() const override {
        return "Replaces standard planar bridge overhangs with self-supporting recursive wavefronts.";
    }
    SubsystemHook get_target_hook() const override { return SubsystemHook::OverhangStrategy; }

    bool on_load(ModuleContext &ctx) override {
        ctx.log_info("Initializing Wave Overhangs Mod...");
        ctx.register_overhang_strategy(std::make_shared<WaveOverhangStrategy>());
        return true;
    }

    void on_unload() override {
        // Cleanup if needed
    }
};

} // namespace Slic3r::Modular

// Export create_module for external dynamic shared library loading
#if defined(_WIN32)
#define ORCA_MOD_EXPORT __declspec(dllexport)
#else
#define ORCA_MOD_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
    ORCA_MOD_EXPORT Slic3r::Modular::IOrcaMod* create_module() {
        return new Slic3r::Modular::WaveOverhangMod();
    }
}
