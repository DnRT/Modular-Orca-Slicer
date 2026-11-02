#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include "compat/expected.hpp"

namespace orca::orchestrator {

namespace fs = std::filesystem;

struct FrankensteinProfile {
    std::string id;
    std::string name;
    std::string description;
    std::string author{"Maker"};
    std::vector<std::string> tags;
    std::string slicer_bin{R"(C:\Program Files\OrcaSlicer\orca-slicer.exe)"};
    std::vector<std::string> active_plugins;
    std::vector<std::pair<std::string, std::string>> env_vars;
    std::vector<std::string> extra_args;

    fs::path get_data_dir(const fs::path &base_dir) const {
        return base_dir / "environments" / id / "data";
    }

    fs::path get_instance_dir(const fs::path &base_dir) const {
        return base_dir / "environments" / id;
    }
};

class FrankensteinManager {
public:
    explicit FrankensteinManager(fs::path base_dir);

    std::vector<FrankensteinProfile> list_profiles() const;
    std::optional<FrankensteinProfile> get_profile(const std::string &id) const;

    compat::expected<FrankensteinProfile, std::string> create_profile(
        const std::string &id,
        const std::string &name,
        const std::string &description = "",
        const std::vector<std::string> &tags = {"maker", "custom"},
        const std::vector<std::string> &plugins = {}
    );

private:
    fs::path m_base_dir;
    fs::path m_envs_dir;
};

} // namespace orca::orchestrator
