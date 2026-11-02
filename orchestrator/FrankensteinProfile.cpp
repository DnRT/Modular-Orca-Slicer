#include "FrankensteinProfile.hpp"
#include <fstream>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

namespace orca::orchestrator {

FrankensteinManager::FrankensteinManager(fs::path base_dir)
    : m_base_dir(std::move(base_dir)), m_envs_dir(m_base_dir / "environments")
{
    fs::create_directories(m_envs_dir);
}

std::vector<FrankensteinProfile> FrankensteinManager::list_profiles() const {
    std::vector<FrankensteinProfile> profiles;
    if (!fs::exists(m_envs_dir)) return profiles;

    for (const auto &entry : fs::directory_iterator(m_envs_dir)) {
        if (entry.is_directory()) {
            fs::path manifest_path = entry.path() / "instance.json";
            if (fs::exists(manifest_path)) {
                try {
                    std::ifstream f(manifest_path);
                    json j;
                    f >> j;

                    FrankensteinProfile p;
                    p.id = j.value("id", entry.path().filename().string());
                    p.name = j.value("name", p.id);
                    p.description = j.value("description", "");
                    p.author = j.value("author", "Maker");
                    p.slicer_bin = j.value("slicer_bin", R"(C:\Program Files\OrcaSlicer\orca-slicer.exe)");
                    p.tags = j.value("tags", std::vector<std::string>{});
                    p.active_plugins = j.value("active_plugins", std::vector<std::string>{});
                    profiles.push_back(std::move(p));
                } catch (...) {
                    // Skip malformed profiles
                }
            }
        }
    }
    return profiles;
}

std::optional<FrankensteinProfile> FrankensteinManager::get_profile(const std::string &id) const {
    fs::path manifest_path = m_envs_dir / id / "instance.json";
    if (!fs::exists(manifest_path)) return std::nullopt;

    try {
        std::ifstream f(manifest_path);
        json j;
        f >> j;

        FrankensteinProfile p;
        p.id = j.value("id", id);
        p.name = j.value("name", p.id);
        p.description = j.value("description", "");
        p.author = j.value("author", "Maker");
        p.slicer_bin = j.value("slicer_bin", R"(C:\Program Files\OrcaSlicer\orca-slicer.exe)");
        p.tags = j.value("tags", std::vector<std::string>{});
        p.active_plugins = j.value("active_plugins", std::vector<std::string>{});
        return p;
    } catch (...) {
        return std::nullopt;
    }
}

compat::expected<FrankensteinProfile, std::string> FrankensteinManager::create_profile(
    const std::string &id,
    const std::string &name,
    const std::string &description,
    const std::vector<std::string> &tags,
    const std::vector<std::string> &plugins)
{
    fs::path inst_dir = m_envs_dir / id;
    fs::path data_dir = inst_dir / "data";
    std::error_code ec;
    fs::create_directories(data_dir, ec);
    if (ec) {
        return compat::unexpected("Failed to create profile directories: " + ec.message());
    }

    FrankensteinProfile p;
    p.id = id;
    p.name = name;
    p.description = description;
    p.tags = tags;
    p.active_plugins = plugins;

    json j;
    j["id"] = p.id;
    j["name"] = p.name;
    j["description"] = p.description;
    j["author"] = p.author;
    j["tags"] = p.tags;
    j["slicer_bin"] = p.slicer_bin;
    j["active_plugins"] = p.active_plugins;

    std::ofstream f(inst_dir / "instance.json");
    if (!f.is_open()) {
        return compat::unexpected("Could not write instance.json");
    }
    f << j.dump(2);

    // Direct launcher for Windows
#if defined(_WIN32)
    std::ofstream bat(inst_dir / "launch.bat");
    if (bat.is_open()) {
        bat << "@echo off\n";
        bat << "title OrcaSlicer Frankenstein - " << name << "\n";
        bat << "\"" << m_base_dir / "OrcaOrchestrator.exe" << "\" launch " << id << "\n";
        bat << "pause\n";
    }
#else
    // Direct shell launcher for Linux
    std::ofstream sh(inst_dir / "launch.sh");
    if (sh.is_open()) {
        sh << "#!/usr/bin/env bash\n";
        sh << "\"" << m_base_dir / "orca-orchestrator" << "\" launch " << id << "\n";
        fs::permissions(inst_dir / "launch.sh", fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);
    }
#endif

    return p;
}

} // namespace orca::orchestrator
