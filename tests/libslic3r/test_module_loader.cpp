#include <catch2/catch.hpp>
#include <fstream>
#include <libslic3r/Modular/ModuleManager.hpp>
#include <libslic3r/Modular/Strategies/WaveOverhangs/WaveOverhangMod.hpp>

using namespace Slic3r::Modular;

TEST_CASE("Mod Loader: Native Mod Registration and Hook Replacement", "[modloader]") {
    auto &mgr = ModuleManager::instance();
    mgr.unload_all();

    SECTION("Direct mod registration replaces OverhangStrategy subsystem") {
        REQUIRE(mgr.get_active_overhang_strategy() == nullptr);

        auto wave_mod = std::make_shared<WaveOverhangMod>();
        bool ok = mgr.register_mod(wave_mod);
        REQUIRE(ok);
        REQUIRE(mgr.has_mod("wave_overhangs"));

        auto strategy = mgr.get_active_overhang_strategy();
        REQUIRE(strategy != nullptr);
        REQUIRE(strategy->id() == "wave_overhangs");
    }

    SECTION("Loading modpack.json manifest dynamically") {
        mgr.unload_all();

        // Write a test modpack.json manifest
        std::string manifest_content = R"({
            "modpack_id": "test_frankenstein_pack",
            "name": "Test Frankenstein Pack",
            "mods": [
                {
                    "id": "wave_overhangs",
                    "enabled": true,
                    "replaces": "OverhangStrategy"
                }
            ]
        })";

        fs::path tmp_manifest = fs::temp_directory_path() / "test_modpack.json";
        {
            std::ofstream f(tmp_manifest);
            f << manifest_content;
        }

        bool loaded = mgr.load_modpack(tmp_manifest);
        REQUIRE(loaded);
        REQUIRE(mgr.has_mod("wave_overhangs"));
        REQUIRE(mgr.get_active_overhang_strategy() != nullptr);

        fs::remove(tmp_manifest);
    }
}
