#pragma once

#include <string>
#include <vector>
#include <memory>
#include "libslic3r/Modular/ModuleABI.hpp"
#include "libslic3r/Modular/IOverhangStrategy.hpp"
#include "libslic3r/Modular/StrategyFlags.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Polyline.hpp"

namespace Slic3r::Modular::Templates {

/**
 * @brief Template Plugin: Custom Overhang & Bridging Strategy
 * 
 * Developers can inherit from IOrcaMod (lifecycle/metadata) and IOverhangStrategy (slicing geometry)
 * to implement novel toolpath generators for steep overhangs and floating bridges.
 */
class TemplateOverhangPlugin : public IOrcaMod, public IOverhangStrategy {
public:
    TemplateOverhangPlugin() = default;
    virtual ~TemplateOverhangPlugin() = default;

    // --- IOrcaMod Lifecycle & Identity Metadata ---
    const char* get_id() const override { return "template_custom_overhang"; }
    const char* get_name() const override { return "Custom Geometric Overhang Generator"; }
    const char* get_version() const override { return "1.0.0"; }
    const char* get_author() const override { return "Community Developer"; }
    const char* get_description() const override {
        return "Template demonstrating how to generate custom toolpaths for unsupported areas without standard support structures.";
    }

    SubsystemHook get_target_hook() const override {
        return SubsystemHook::OverhangStrategy;
    }

    bool on_load(ModuleContext &ctx) override {
        m_active = true;
        ctx.log_info("TemplateOverhangPlugin: Successfully initialized and active.");
        return true;
    }

    void on_unload() override {
        m_active = false;
    }

    // --- IOverhangStrategy Implementation ---
    std::string id() const override { return get_id(); }
    std::string name() const override { return get_name(); }
    std::string description() const override { return get_description(); }

    /**
     * @brief Generates custom extrusion paths for the detected overhang regions
     * @param ctx Context containing overhang geometries, lower layer contours, spacing, and flow properties
     * @return OverhangResult containing the generated ExtrusionPaths and covered area footprint
     */
    OverhangResult generate(const OverhangContext &ctx) override {
        OverhangResult result;
        if (ctx.overhang_area.empty() || !m_active) {
            result.success = true;
            return result;
        }

        // Example: Inspect unsupported overhang polygons and synthesize paths
        // std::vector<ExtrusionPaths> custom_paths;
        // for (const ExPolygon &expoly : ctx.overhang_area) {
        //     ... custom algorithmic geometric processing ...
        // }

        result.success = true;
        return result;
    }

private:
    bool m_active{true};
};

} // namespace Slic3r::Modular::Templates
