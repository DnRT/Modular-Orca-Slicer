#pragma once

#include <string>
#include <vector>
#include <memory>
#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/Flow.hpp"
#include "libslic3r/Polygon.hpp"
#include "StrategyFlags.hpp"

namespace Slic3r::Modular {

struct OverhangContext {
    const ExPolygons &overhang_area;
    const Polygons   &lower_slices_polygons;
    int               perimeter_count        = 1;
    double            line_spacing           = 0.35;
    double            line_width             = 0.4;
    Flow              overhang_flow;
    double            scaled_resolution      = 1.0;
    
    // Wave specific & future-strategy tunables
    double            perimeter_overlap      = 0.1;
    double            minimum_wave_width     = 0.7;
    double            min_new_area           = 0.01;
    int               max_iterations         = 0;
    bool              use_instead_of_bridges = false;
    
    // Corner-aware taper parameters
    bool              corner_taper_enable    = false;
    double            line_spacing_corner    = 0.0;
    double            corner_taper_distance  = 0.0;
    double            corner_angle_threshold = 90.0;
};

struct OverhangResult {
    std::vector<ExtrusionPaths> paths;
    Polygons                    residual; // Area covered to subtract from normal infill
    Polygons                    covered_footprint;
    bool                        success = true;
};

class IOverhangStrategy {
public:
    virtual ~IOverhangStrategy() = default;

    virtual std::string id() const = 0;
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;

    // Core generator method
    virtual OverhangResult generate(const OverhangContext &ctx) = 0;
};

} // namespace Slic3r::Modular
