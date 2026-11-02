#include "FrankensteinProfile.hpp"
#include <fstream>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

namespace orca::orchestrator {

FrankensteinManager::FrankensteinManager(fs::path base_dir)
    : m_base_dir(std::move(base_dir))
    , m_envs_dir(m_base_dir / "environments")
    , m_mods_catalog_dir(m_base_dir / "mods_catalog")
{
    fs::create_directories(m_envs_dir);
    fs::create_directories(m_mods_catalog_dir);
    ensure_built_in_mods();
}

void FrankensteinManager::ensure_built_in_mods() {
    // 1. Wave Overhangs
    fs::path wave_json = m_mods_catalog_dir / "wave_overhangs.json";
    if (!fs::exists(wave_json)) {
        json j;
        j["id"] = "wave_overhangs";
        j["name"] = "Wave Overhangs (Andersons/Klappe)";
        j["description"] = "Replaces base overhang system with self-supporting recursive wavefronts.";
        j["author"] = "Janis Andersons / Dennis Klappe";
        j["target_hook"] = "OverhangStrategy";
        j["library_path"] = ""; // built-in modular strategy
        std::ofstream f(wave_json);
        f << j.dump(2);
    }

    // 2. Arc Overhangs (Community mod entry)
    fs::path arc_json = m_mods_catalog_dir / "arc_overhangs.json";
    if (!fs::exists(arc_json)) {
        json j;
        j["id"] = "arc_overhangs";
        j["name"] = "Arc Overhangs (McCulloch)";
        j["description"] = "Replaces base overhangs with circular arc bridging patterns.";
        j["author"] = "Steven McCulloch";
        j["target_hook"] = "OverhangStrategy";
        j["library_path"] = "";
        std::ofstream f(arc_json);
        f << j.dump(2);
    }

    // 3. Non-Planar 3-Axis Continuous Z
    fs::path np_json = m_mods_catalog_dir / "non_planar_3axis.json";
    if (!fs::exists(np_json)) {
        json j;
        j["id"] = "non_planar_3axis";
        j["name"] = "Non-Planar 3-Axis Kinematics";
        j["description"] = "Augments layer generation with simultaneous continuous 3D XYZ movements.";
        j["author"] = "RepRap Maker Community";
        j["target_hook"] = "KinematicsEngine";
        j["library_path"] = "";
        std::ofstream f(np_json);
        f << j.dump(2);
    }

    // 4. Lightning Infill v2
    fs::path li_json = m_mods_catalog_dir / "lightning_infill_v2.json";
    if (!fs::exists(li_json)) {
        json j;
        j["id"] = "lightning_infill_v2";
        j["name"] = "RepRap Lightning Infill v2";
        j["description"] = "Replaces internal infill calculation with procedural organic tree supports.";
        j["author"] = "Maker Community";
        j["target_hook"] = "InfillStrategy";
        j["library_path"] = "";
        std::ofstream f(li_json);
        f << j.dump(2);
    }
}

std::optional<ModDescriptor> FrankensteinManager::get_mod(const std::string &id) const {
    auto mods = list_available_mods();
    for (const auto &m : mods) {
        if (m.id == id) return m;
    }
    return std::nullopt;
}

std::vector<ModDescriptor> FrankensteinManager::list_available_mods() const {
    std::vector<ModDescriptor> mods;
    if (!fs::exists(m_mods_catalog_dir)) return mods;

    for (const auto &entry : fs::directory_iterator(m_mods_catalog_dir)) {
        if (entry.path().extension() == ".json") {
            try {
                std::ifstream f(entry.path());
                json j;
                f >> j;

                ModDescriptor m;
                m.id = j.value("id", entry.path().stem().string());
                m.name = j.value("name", m.id);
                m.description = j.value("description", "");
                m.author = j.value("author", "Community");
                m.target_hook = j.value("target_hook", "General");
                m.library_path = j.value("library_path", "");
                m.enabled = true;
                mods.push_back(std::move(m));
            } catch (...) {}
        }
    }
    return mods;
}

std::vector<ModpackInstance> FrankensteinManager::list_modpacks() const {
    std::vector<ModpackInstance> packs;
    if (!fs::exists(m_envs_dir)) return packs;

    for (const auto &entry : fs::directory_iterator(m_envs_dir)) {
        if (entry.is_directory()) {
            fs::path manifest = entry.path() / "instance.json";
            if (fs::exists(manifest)) {
                try {
                    std::ifstream f(manifest);
                    json j;
                    f >> j;

                    ModpackInstance pack;
                    pack.id = j.value("id", entry.path().filename().string());
                    pack.name = j.value("name", pack.id);
                    pack.description = j.value("description", "");
                    pack.author = j.value("author", "Maker");
                    pack.slicer_bin = j.value("slicer_bin", R"(C:\Program Files\OrcaSlicer\orca-slicer.exe)");
                    pack.tags = j.value("tags", std::vector<std::string>{});
                    
                    if (j.contains("mods") && j["mods"].is_array()) {
                        for (const auto &m_json : j["mods"]) {
                            ModDescriptor m;
                            m.id = m_json.value("id", "");
                            m.name = m_json.value("name", m.id);
                            m.target_hook = m_json.value("target_hook", "");
                            m.library_path = m_json.value("library_path", "");
                            m.enabled = m_json.value("enabled", true);
                            pack.mods.push_back(std::move(m));
                        }
                    }
                    packs.push_back(std::move(pack));
                } catch (...) {}
            }
        }
    }
    return packs;
}

std::optional<ModpackInstance> FrankensteinManager::get_modpack(const std::string &id) const {
    fs::path manifest = m_envs_dir / id / "instance.json";
    if (!fs::exists(manifest)) return std::nullopt;

    try {
        std::ifstream f(manifest);
        json j;
        f >> j;

        ModpackInstance pack;
        pack.id = j.value("id", id);
        pack.name = j.value("name", pack.id);
        pack.description = j.value("description", "");
        pack.author = j.value("author", "Maker");
        pack.slicer_bin = j.value("slicer_bin", R"(C:\Program Files\OrcaSlicer\orca-slicer.exe)");
        pack.tags = j.value("tags", std::vector<std::string>{});
        
        if (j.contains("mods") && j["mods"].is_array()) {
            for (const auto &m_json : j["mods"]) {
                ModDescriptor m;
                m.id = m_json.value("id", "");
                m.name = m_json.value("name", m.id);
                m.target_hook = m_json.value("target_hook", "");
                m.library_path = m_json.value("library_path", "");
                m.enabled = m_json.value("enabled", true);
                pack.mods.push_back(std::move(m));
            }
        }
        return pack;
    } catch (...) {
        return std::nullopt;
    }
}

compat::expected<ModpackInstance, std::string> FrankensteinManager::create_modpack(
    const std::string &id,
    const std::string &name,
    const std::string &description,
    const std::vector<std::string> &tags)
{
    fs::path inst_dir = m_envs_dir / id;
    fs::create_directories(inst_dir / "data");

    ModpackInstance pack;
    pack.id = id;
    pack.name = name;
    pack.description = description;
    pack.tags = tags;

    json j;
    j["id"] = pack.id;
    j["name"] = pack.name;
    j["description"] = pack.description;
    j["author"] = pack.author;
    j["tags"] = pack.tags;
    j["slicer_bin"] = pack.slicer_bin;
    j["mods"] = json::array();

    std::ofstream f(inst_dir / "instance.json");
    if (!f.is_open()) return compat::unexpected("Failed to write instance.json");
    f << j.dump(2);

    return pack;
}

compat::expected<bool, std::string> FrankensteinManager::add_mod(const std::string &pack_id, const std::string &mod_id) {
    auto pack_opt = get_modpack(pack_id);
    if (!pack_opt) return compat::unexpected("Modpack '" + pack_id + "' not found");

    auto &pack = *pack_opt;
    for (const auto &m : pack.mods) {
        if (m.id == mod_id) return compat::unexpected("Mod '" + mod_id + "' is already in this modpack!");
    }

    // Find mod in catalog
    auto available = list_available_mods();
    const ModDescriptor* found = nullptr;
    for (const auto &av : available) {
        if (av.id == mod_id) {
            found = &av;
            break;
        }
    }
    if (!found) return compat::unexpected("Mod '" + mod_id + "' not found in catalog");

    pack.mods.push_back(*found);

    // Save instance.json
    fs::path manifest = m_envs_dir / pack_id / "instance.json";
    json j;
    j["id"] = pack.id;
    j["name"] = pack.name;
    j["description"] = pack.description;
    j["author"] = pack.author;
    j["tags"] = pack.tags;
    j["slicer_bin"] = pack.slicer_bin;
    j["mods"] = json::array();

    for (const auto &m : pack.mods) {
        json mj;
        mj["id"] = m.id;
        mj["name"] = m.name;
        mj["target_hook"] = m.target_hook;
        mj["library_path"] = m.library_path;
        mj["enabled"] = m.enabled;
        j["mods"].push_back(mj);
    }

    std::ofstream f(manifest);
    f << j.dump(2);
    return true;
}

compat::expected<bool, std::string> FrankensteinManager::remove_mod(const std::string &pack_id, const std::string &mod_id) {
    auto pack_opt = get_modpack(pack_id);
    if (!pack_opt) return compat::unexpected("Modpack '" + pack_id + "' not found");

    auto &pack = *pack_opt;
    bool removed = false;
    std::vector<ModDescriptor> updated;
    for (auto &m : pack.mods) {
        if (m.id == mod_id) {
            removed = true;
        } else {
            updated.push_back(m);
        }
    }

    if (!removed) return compat::unexpected("Mod '" + mod_id + "' was not in modpack");

    pack.mods = updated;

    fs::path manifest = m_envs_dir / pack_id / "instance.json";
    json j;
    j["id"] = pack.id;
    j["name"] = pack.name;
    j["description"] = pack.description;
    j["author"] = pack.author;
    j["tags"] = pack.tags;
    j["slicer_bin"] = pack.slicer_bin;
    j["mods"] = json::array();

    for (const auto &m : pack.mods) {
        json mj;
        mj["id"] = m.id;
        mj["name"] = m.name;
        mj["target_hook"] = m.target_hook;
        mj["library_path"] = m.library_path;
        mj["enabled"] = m.enabled;
        j["mods"].push_back(mj);
    }

    std::ofstream f(manifest);
    f << j.dump(2);
    return true;
}

compat::expected<fs::path, std::string> FrankensteinManager::generate_manifest(const std::string &pack_id) {
    auto pack_opt = get_modpack(pack_id);
    if (!pack_opt) return compat::unexpected("Modpack '" + pack_id + "' not found");

    auto &pack = *pack_opt;
    fs::path out_path = pack.get_modpack_manifest_path(m_base_dir);

    json j;
    j["modpack_id"] = pack.id;
    j["name"] = pack.name;
    j["target_engine"] = "OrcaSlicer 2.5.0-dev";
    j["mods"] = json::array();

    for (const auto &m : pack.mods) {
        if (!m.enabled) continue;
        json mj;
        mj["id"] = m.id;
        mj["name"] = m.name;
        mj["replaces"] = m.target_hook;
        mj["library_path"] = m.library_path;
        mj["enabled"] = true;
        j["mods"].push_back(mj);
    }

    std::ofstream f(out_path);
    if (!f.is_open()) return compat::unexpected("Could not write modpack.json");
    f << j.dump(2);

    return out_path;
}

} // namespace orca::orchestrator
