#pragma once

#include "../../IOverhangStrategy.hpp"
#include "WaveAlgorithm.hpp"

namespace Slic3r::Modular {

class WaveOverhangStrategy : public IOverhangStrategy {
public:
    std::string id() const override { return "wave_overhangs"; }
    std::string name() const override { return "Wave Overhangs (Andersons/Klappe)"; }
    std::string description() const override { 
        return "Generates self-supporting expanding wavefronts for steep and 90-degree overhangs without support material.";
    }

    OverhangResult generate(const OverhangContext &ctx) override;
};

} // namespace Slic3r::Modular
