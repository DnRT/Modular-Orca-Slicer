#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include "compat/expected.hpp"

namespace orca::orchestrator {

namespace fs = std::filesystem;

struct ModDescriptor {
    std::string id;
    std::string name;
    std::string description;
    std::string author;
    std::string target_hook; // e.g. "OverhangStrategy", "KinematicsEngine"
    std::string library_path; // optional path to .dll / .so (or built-in)
    bool enabled{true};
};

struct ModpackInstance {
    std::string id;
    std::string name;
    std::string description;
    std::string author{"Maker"};
    std::vector<std::string> tags;
    std::string slicer_bin{R"(modular-orca-slicer.exe)"};
    std::vector<ModDescriptor> mods;
    std::vector<std::string> extra_args;

    fs::path get_dir(const fs::path &base_dir) const {
        return base_dir / "environments" / id;
    }

    fs::path get_data_dir(const fs::path &base_dir) const {
        return get_dir(base_dir) / "data";
    }

    fs::path get_modpack_manifest_path(const fs::path &base_dir) const {
        return get_dir(base_dir) / "modpack.json";
    }
};

class FrankensteinManager {
public:
    explicit FrankensteinManager(fs::path base_dir);

    // Mod catalog in ecosystem
    std::vector<ModDescriptor> list_available_mods() const;
    std::optional<ModDescriptor> get_mod(const std::string &id) const;

    // Modpack instances
    std::vector<ModpackInstance> list_modpacks() const;
    std::optional<ModpackInstance> get_modpack(const std::string &id) const;

    compat::expected<ModpackInstance, std::string> create_modpack(
        const std::string &id,
        const std::string &name,
        const std::string &description = "",
        const std::vector<std::string> &tags = {"maker", "custom"}
    );

    compat::expected<bool, std::string> add_mod(const std::string &pack_id, const std::string &mod_id);
    compat::expected<bool, std::string> remove_mod(const std::string &pack_id, const std::string &mod_id);

    // Generates the active modpack.json manifest to be loaded by OrcaSlicer's ModuleManager
    compat::expected<fs::path, std::string> generate_manifest(const std::string &pack_id);

private:
    fs::path m_base_dir;
    fs::path m_envs_dir;
    fs::path m_mods_catalog_dir;

    void ensure_built_in_mods();
};

} // namespace orca::orchestrator
