#pragma once

#include <string>
#include <vector>
#include "libslic3r/Modular/ModuleABI.hpp"
#include "IWallInfillStrategy.hpp"

namespace Slic3r::Modular {

// Built-in Modular implementation of Wall & Infill enhancer
class WallInfillMod : public IOrcaMod, public IWallInfillStrategy {
public:
    WallInfillMod() = default;
    virtual ~WallInfillMod() = default;

    // IOrcaMod
    const char* get_id() const override { return "modular_infill_walls"; }
    const char* get_name() const override { return "Algorithmic Walls & Infill Enhancer"; }
    const char* get_version() const override { return "1.0.0"; }
    const char* get_author() const override { return "Modular RepRap Community"; }
    const char* get_description() const override {
        return "Enhances perimeter and infill path generation with non-crossing travel optimization and continuous bridges.";
    }

    SubsystemHook get_target_hook() const override {
        return SubsystemHook::InfillPattern;
    }

    bool on_load(ModuleContext &ctx) override {
        m_active = true;
        ctx.log_info("Wall/Infill Enhancer Active: Algorithmic path optimization hooked.");
        return true;
    }

    void on_unload() override {
        m_active = false;
    }

    // IWallInfillStrategy
    std::string id() const override { return get_id(); }
    std::string name() const override { return get_name(); }
    WallInfillAlgorithm get_algorithm() const override { return WallInfillAlgorithm::AdaptiveNonCrossing; }
    bool should_damp_travel_moves() const override { return m_active; }

private:
    bool m_active{false};
};

} // namespace Slic3r::Modular
