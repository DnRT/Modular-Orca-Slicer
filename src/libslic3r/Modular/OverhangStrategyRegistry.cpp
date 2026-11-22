#include "OverhangStrategyRegistry.hpp"
#include "Strategies/WaveOverhangs/WaveOverhangStrategy.hpp"
#include "Strategies/ArcOverhangs/ArcOverhangStrategy.hpp"

namespace Slic3r::Modular {

OverhangStrategyRegistry& OverhangStrategyRegistry::instance() {
    static OverhangStrategyRegistry s_instance;
    return s_instance;
}

OverhangStrategyRegistry::OverhangStrategyRegistry() {
    // Automatically register built-in community modular strategies
    register_strategy(std::make_shared<WaveOverhangStrategy>());
    register_strategy(std::make_shared<ArcOverhangStrategy>());
}

void OverhangStrategyRegistry::register_strategy(std::shared_ptr<IOverhangStrategy> strategy) {
    if (strategy) {
        m_strategies[strategy->id()] = strategy;
    }
}

std::shared_ptr<IOverhangStrategy> OverhangStrategyRegistry::get_strategy(const std::string &id) const {
    auto it = m_strategies.find(id);
    if (it != m_strategies.end()) {
        return it->second;
    }
    return nullptr;
}

bool OverhangStrategyRegistry::has_strategy(const std::string &id) const {
    return m_strategies.find(id) != m_strategies.end();
}

std::vector<std::string> OverhangStrategyRegistry::get_available_ids() const {
    std::vector<std::string> ids;
    ids.reserve(m_strategies.size());
    for (const auto &pair : m_strategies) {
        ids.push_back(pair.first);
    }
    return ids;
}

} // namespace Slic3r::Modular
