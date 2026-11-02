#include "WaveOverhangStrategy.hpp"

namespace Slic3r::Modular {

OverhangResult WaveOverhangStrategy::generate(const OverhangContext &ctx) {
    OverhangResult res;
    if (ctx.overhang_area.empty()) {
        res.success = true;
        return res;
    }

    WaveOverhangs::CommonParams params;
    params.perimeter_count        = ctx.perimeter_count;
    params.line_spacing           = ctx.line_spacing;
    params.line_width             = ctx.line_width;
    params.overhang_flow          = ctx.overhang_flow;
    params.scaled_resolution      = ctx.scaled_resolution;
    params.perimeter_overlap      = ctx.perimeter_overlap;
    params.minimum_wave_width     = ctx.minimum_wave_width;
    params.min_new_area           = ctx.min_new_area;
    params.max_iterations         = ctx.max_iterations;
    params.use_instead_of_bridges = ctx.use_instead_of_bridges;
    params.corner_taper_enable    = ctx.corner_taper_enable;
    params.line_spacing_corner    = ctx.line_spacing_corner;
    params.corner_taper_distance  = ctx.corner_taper_distance;
    params.corner_angle_threshold = ctx.corner_angle_threshold;

    auto wave_res = WaveOverhangs::generate(ctx.overhang_area, ctx.lower_slices_polygons, params);

    res.paths = std::move(wave_res.paths);
    res.residual = std::move(wave_res.residual);
    res.covered_footprint = res.residual;
    res.success = !res.paths.empty();

    return res;
}

} // namespace Slic3r::Modular
