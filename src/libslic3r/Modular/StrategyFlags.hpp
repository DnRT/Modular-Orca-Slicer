#pragma once

#include <cstdint>

namespace Slic3r::Modular {

// Bitmask flags for extensible extrusion path classification across any plugin/strategy
enum StrategyPathFlags : uint32_t {
    FLAG_NONE                  = 0,
    FLAG_WAVE_OVERHANG         = 1 << 0,  // Path generated as a wave overhang trace
    FLAG_WAVE_FLOOR            = 1 << 1,  // Path is solid infill on a floor layer above a wave region
    FLAG_WAVE_PERIMETER        = 1 << 2,  // Perimeter wall that anchors against wave traces
    FLAG_WAVE_FLOOR_PERIMETER  = 1 << 3,  // Perimeter on a floor layer above a wave region
    FLAG_NON_PLANAR_3D         = 1 << 4,  // Continuous 3-axis non-planar motion
    FLAG_ARC_OVERHANG          = 1 << 5,  // Arc overhang path
    FLAG_VARIABLE_FLOW_SPECIAL = 1 << 6   // Special dynamic volumetric modulation
};

inline bool has_flag(uint32_t flags, StrategyPathFlags check) {
    return (flags & static_cast<uint32_t>(check)) != 0;
}

inline void set_flag(uint32_t &flags, StrategyPathFlags add) {
    flags |= static_cast<uint32_t>(add);
}

} // namespace Slic3r::Modular
