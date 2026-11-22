#pragma once

#include <iostream>
#include <vector>
#include "libslic3r/Modular/ModuleABI.hpp"
#include "libslic3r/Layer.hpp"

namespace Slic3r::Modular {

// Community Plugin: Eager Layer Buffer Flusher
// Proactively flushes intermediate slice geometries (raw_slices, unneeded contour tables)
// as each layer completes its perimeter and infill calculations.
// Universal across all nozzle diameters (0.2mm to 1.0mm) and layer counts (100 to 2000+ layers),
// preventing continuous linear memory accumulation and stabilizing the RAM curve during massive slices.
class EagerLayerFlusherMod : public IOrcaMod {
public:
    EagerLayerFlusherMod() = default;
    virtual ~EagerLayerFlusherMod() = default;

    const char* get_id() const override { return "eager_layer_flusher"; }
    const char* get_name() const override { return "Eager Layer Buffer Flusher"; }
    const char* get_version() const override { return "1.0.0"; }
    const char* get_author() const override { return "Modular RepRap Community"; }
    const char* get_description() const override {
        return "Eagerly reclaims RAM by releasing intermediate slice contour buffers after perimeters and infills are generated. Universal for any nozzle size.";
    }

    SubsystemHook get_target_hook() const override {
        return SubsystemHook::ResourceOptimizer;
    }

    bool on_load(ModuleContext &ctx) override {
        m_active = true;
        ctx.log_info("Eager Layer Flusher Active: Rolling memory reclamation enabled.");
        return true;
    }

    void on_unload() override {
        m_active = false;
    }

    bool is_active() const { return m_active; }

    // Proactively flushes non-essential intermediate buffers for a completed layer
    void flush_layer_buffers(Layer *layer) const {
        if (!m_active || !layer) return;

        // Eagerly release intermediate raw_slices and non-overlap fill boundary buffers from all regions
        for (LayerRegion *layerm : layer->regions()) {
            if (!layerm) continue;

            if (!layerm->raw_slices.empty()) {
                layerm->raw_slices.clear();
                layerm->raw_slices.shrink_to_fit();
            }

            if (!layerm->fill_no_overlap_expolygons.empty()) {
                layerm->fill_no_overlap_expolygons.clear();
                layerm->fill_no_overlap_expolygons.shrink_to_fit();
            }
        }
    }

private:
    bool m_active{true};
};

} // namespace Slic3r::Modular
