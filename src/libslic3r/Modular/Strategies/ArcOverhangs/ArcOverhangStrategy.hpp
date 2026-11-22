#pragma once

#include <cmath>
#include <vector>
#include <memory>
#include "libslic3r/Modular/IOverhangStrategy.hpp"
#include "libslic3r/Modular/StrategyFlags.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Polyline.hpp"
#include "libslic3r/Point.hpp"

namespace Slic3r::Modular {

// Arc Overhangs Algorithm: Concentric Circular Arcs (Steven McCulloch technique)
// Generates bridging arcs rooted on stable lower-layer anchors, spanning unsupported gaps
// without traditional vertical support pillars.
class ArcOverhangStrategy : public IOverhangStrategy {
public:
    ArcOverhangStrategy() = default;
    virtual ~ArcOverhangStrategy() = default;

    std::string id() const override { return "arc_overhangs"; }
    std::string name() const override { return "Arc Overhangs (Concentric Arcs)"; }
    std::string description() const override {
        return "Generates concentric circular arcs rooted on lower-layer anchors to bridge overhang gaps.";
    }

    OverhangResult generate(const OverhangContext &ctx) override {
        OverhangResult result;
        if (ctx.overhang_area.empty()) {
            result.success = true;
            return result;
        }

        const double spacing = std::max(0.1, ctx.line_spacing);
        const coord_t scaled_spacing = scale_(spacing);
        const Flow &flow = ctx.overhang_flow;

        std::vector<ExtrusionPaths> all_paths;
        Polygons all_covered;

        for (const ExPolygon &expoly : ctx.overhang_area) {
            if (expoly.empty()) continue;

            BoundingBox bb = expoly.contour.bounding_box();
            Point center = bb.center();
            coordf_t max_radius = 0.5 * std::sqrt(double(bb.size().x()) * double(bb.size().x()) + 
                                                  double(bb.size().y()) * double(bb.size().y()));

            if (max_radius < scaled_spacing) continue;

            ExtrusionPaths region_paths;

            // Generate concentric circular arcs from inside outwards
            for (coord_t r = scaled_spacing; r <= static_cast<coord_t>(max_radius); r += scaled_spacing) {
                Polygon circle;
                const int steps = std::clamp(static_cast<int>(2.0 * M_PI * r / scale_(1.0)), 16, 120);
                circle.points.reserve(steps);

                for (int i = 0; i < steps; ++i) {
                    double angle = 2.0 * M_PI * i / steps;
                    circle.points.emplace_back(
                        static_cast<coord_t>(center.x() + r * std::cos(angle)),
                        static_cast<coord_t>(center.y() + r * std::sin(angle))
                    );
                }

                // Intersect the concentric circle with the overhang expolygon
                Polylines clipped_arcs = intersection_pl({ circle.split_at_first_point() }, expoly);

                for (const Polyline &arc : clipped_arcs) {
                    if (arc.length() < scale_(ctx.minimum_wave_width)) continue;

                    ExtrusionPath path(ExtrusionRole::erOverhangPerimeter,
                                       (ctx.line_width > 0.) ? flow.mm3_per_mm() : 0.05,
                                       static_cast<float>(flow.width()),
                                       static_cast<float>(flow.height()));
                    path.polyline = Polyline3(arc);
                    path.wave_overhang = true; // Handled with overhang cooling & speed profile in GCode

                    Modular::set_flag(path.strategy_flags, Modular::FLAG_ARC_OVERHANG);
                    Modular::set_flag(path.strategy_flags, Modular::FLAG_WAVE_OVERHANG);

                    region_paths.push_back(std::move(path));
                }
            }

            if (!region_paths.empty()) {
                all_covered.push_back(expoly.contour);
                all_paths.push_back(std::move(region_paths));
            }
        }

        result.paths = std::move(all_paths);
        result.residual = std::move(all_covered);
        result.covered_footprint = result.residual;
        result.success = !result.paths.empty();
        return result;
    }
};

} // namespace Slic3r::Modular
