#pragma once

#include <string>
#include <vector>

namespace Slic3r::Modular {

enum class WallInfillAlgorithm {
    ClassicPerimeter,
    ArachnePerimeter,
    AdaptiveNonCrossing,
    ContinuousHilbert
};

// Interface for customizable wall and infill generation algorithms
class IWallInfillStrategy {
public:
    virtual ~IWallInfillStrategy() = default;

    virtual std::string id() const = 0;
    virtual std::string name() const = 0;
    virtual WallInfillAlgorithm get_algorithm() const = 0;

    // Evaluates whether adaptive damping should be applied to avoid nozzle dragging
    virtual bool should_damp_travel_moves() const = 0;
};

} // namespace Slic3r::Modular
