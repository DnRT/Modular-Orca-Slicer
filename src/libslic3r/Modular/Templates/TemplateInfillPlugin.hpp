#pragma once

#include <string>
#include <vector>
#include <memory>
#include "libslic3r/Modular/ModuleABI.hpp"
#include "libslic3r/Modular/Strategies/WallInfill/IWallInfillStrategy.hpp"

namespace Slic3r::Modular::Templates {

/**
 * @brief Template Plugin: Custom Infill / Wall Algorithm
 * 
 * Developers can inherit from IOrcaMod (lifecycle/metadata) and IWallInfillStrategy
 * to integrate new toolpath generation strategies (e.g., custom fractals, non-crossing perimeters).
 */
class TemplateInfillPlugin : public IOrcaMod, public IWallInfillStrategy {
public:
    TemplateInfillPlugin() = default;
    virtual ~TemplateInfillPlugin() = default;

    // --- IOrcaMod Lifecycle & Identity Metadata ---
    const char* get_id() const override { return "template_custom_infill"; }
    const char* get_name() const override { return "Custom Hilbert/Wall Generator"; }
    const char* get_version() const override { return "1.0.0"; }
    const char* get_author() const override { return "Community Developer"; }
    const char* get_description() const override {
        return "Template demonstrating custom infill pattern or wall generation strategy hook.";
    }

    SubsystemHook get_target_hook() const override {
        return SubsystemHook::InfillPattern;
    }

    bool on_load(ModuleContext &ctx) override {
        m_active = true;
        ctx.log_info("TemplateInfillPlugin: Initialized successfully.");
        return true;
    }

    void on_unload() override {
        m_active = false;
    }

    // --- IWallInfillStrategy Implementation ---
    std::string id() const override { return get_id(); }
    std::string name() const override { return get_name(); }

    WallInfillAlgorithm get_algorithm() const override {
        return WallInfillAlgorithm::AdaptiveNonCrossing;
    }

    bool should_damp_travel_moves() const override {
        // Return true if travel moves should be smoothed or damped across walls
        return true;
    }

private:
    bool m_active{true};
};

} // namespace Slic3r::Modular::Templates
