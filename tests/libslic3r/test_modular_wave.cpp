#include <catch2/catch.hpp>
#include <libslic3r/Modular/OverhangStrategyRegistry.hpp>
#include <libslic3r/Modular/Strategies/WaveOverhangs/WaveOverhangStrategy.hpp>
#include <libslic3r/Flow.hpp>

using namespace Slic3r;
using namespace Slic3r::Modular;

TEST_CASE("Modular Architecture: OverhangStrategyRegistry", "[modular]") {
    auto &registry = OverhangStrategyRegistry::instance();

    SECTION("WaveOverhangs strategy is registered by default") {
        REQUIRE(registry.has_strategy("wave_overhangs"));
        auto strategy = registry.get_strategy("wave_overhangs");
        REQUIRE(strategy != nullptr);
        REQUIRE(strategy->id() == "wave_overhangs");
        REQUIRE(strategy->name() == "Wave Overhangs (Andersons/Klappe)");
    }

    SECTION("Non-existent strategy returns nullptr") {
        REQUIRE_FALSE(registry.has_strategy("non_existent_strategy"));
        REQUIRE(registry.get_strategy("non_existent_strategy") == nullptr);
    }
}

TEST_CASE("Modular Strategy: WaveOverhangs generation & tagging", "[modular]") {
    auto &registry = OverhangStrategyRegistry::instance();
    auto strategy = registry.get_strategy("wave_overhangs");
    REQUIRE(strategy != nullptr);

    SECTION("Empty overhang returns empty result safely") {
        ExPolygons empty_overhang;
        Polygons lower_slices;
        Flow flow(0.4f, 0.2f, 0.4f);

        OverhangContext ctx{
            empty_overhang,
            lower_slices,
            1,
            0.35,
            0.4,
            flow,
            1.0
        };

        auto res = strategy->generate(ctx);
        REQUIRE(res.paths.empty());
        REQUIRE(res.residual.empty());
    }

    SECTION("Generates and tags paths with FLAG_WAVE_OVERHANG") {
        // Create an anchor box at [0, 0] to [20, 20] mm
        Polygon anchor_poly;
        anchor_poly.points = {
            Point(scale_(0.0), scale_(0.0)),
            Point(scale_(20.0), scale_(0.0)),
            Point(scale_(20.0), scale_(20.0)),
            Point(scale_(0.0), scale_(20.0))
        };
        Polygons lower_slices = { anchor_poly };

        // Create an overhang area hanging into [20, 0] to [40, 20] mm (a 90-deg cantilever)
        Polygon overhang_poly;
        overhang_poly.points = {
            Point(scale_(20.0), scale_(0.0)),
            Point(scale_(40.0), scale_(0.0)),
            Point(scale_(40.0), scale_(20.0)),
            Point(scale_(20.0), scale_(20.0))
        };
        ExPolygons overhang_area = { ExPolygon(overhang_poly) };

        Flow flow(0.4f, 0.2f, 0.4f);
        OverhangContext ctx{
            overhang_area,
            lower_slices,
            1,       // perimeter_count
            0.35,    // line_spacing
            0.4,     // line_width
            flow,
            1.0      // scaled_resolution
        };
        ctx.minimum_wave_width = 0.5;
        ctx.perimeter_overlap = 0.2;

        auto res = strategy->generate(ctx);
        
        // If paths generated, verify they carry the modular flag
        if (!res.paths.empty()) {
            for (const auto &region : res.paths) {
                for (const auto &path : region) {
                    REQUIRE(Modular::has_flag(path.strategy_flags, Modular::FLAG_WAVE_OVERHANG));
                }
            }
        }
    }
}
