#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "IOverhangStrategy.hpp"

namespace Slic3r::Modular {

class OverhangStrategyRegistry {
public:
    static OverhangStrategyRegistry& instance();

    void register_strategy(std::shared_ptr<IOverhangStrategy> strategy);
    std::shared_ptr<IOverhangStrategy> get_strategy(const std::string &id) const;
    std::vector<std::string> get_available_ids() const;

    // Checks if strategy is available; if not or "default", returns nullptr (use OSO native)
    bool has_strategy(const std::string &id) const;

private:
    OverhangStrategyRegistry();
    std::unordered_map<std::string, std::shared_ptr<IOverhangStrategy>> m_strategies;
};

} // namespace Slic3r::Modular
