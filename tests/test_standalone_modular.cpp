#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <libslic3r/Modular/ModuleManager.hpp>
#include <libslic3r/Modular/Strategies/WaveOverhangs/WaveAlgorithm.hpp>
#include <libslic3r/Modular/Strategies/WaveOverhangs/WaveOverhangMod.hpp>
#include <libslic3r/Modular/Strategies/WaveOverhangs/WaveOverhangStrategy.hpp>

namespace fs = std::filesystem;
using namespace Slic3r::Modular;

int main() {
    std::cout << "=================================================================\n";
    std::cout << "  VERIFYING MODULAR ORCASLICER & WAVE OVERHANGS SUBSYSTEM\n";
    std::cout << "=================================================================\n\n";

    auto &mgr = ModuleManager::instance();
    mgr.unload_all();

    // Test 1: Verify Initial State
    std::cout << "[Test 1] Checking initial subsystem state: ";
    assert(mgr.get_active_overhang_strategy() == nullptr);
    std::cout << "PASSED (Clean base state)\n";

    // Test 2: Direct Mod Registration
    std::cout << "[Test 2] Registering Wave Overhangs Mod: ";
    auto wave_mod = std::make_shared<WaveOverhangMod>();
    bool ok = mgr.register_mod(wave_mod);
    assert(ok);
    assert(mgr.has_mod("wave_overhangs"));
    auto active_strat = mgr.get_active_overhang_strategy();
    assert(active_strat != nullptr);
    assert(active_strat->id() == "wave_overhangs");
    std::cout << "PASSED (Subsystem [OverhangStrategy] successfully replaced!)\n";

    // Test 3: Wavefront Algorithm Geometry Computation
    std::cout << "[Test 3] Testing Janis Andersons recursive wavefront algorithm: ";
    WaveConfig config;
    config.wave_height = 0.8f;
    config.wave_wavelength = 3.0f;
    config.wave_propagation_rate = 1.0f;
    config.dampening_rate = 0.1f;

    std::vector<WavePoint> input = {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f, 0.0f},
        {20.0f, 0.0f, 0.0f, 0.0f}
    };
    auto propagated = WaveAlgorithm::propagate_wavefront(input, config, 0.2f);
    assert(!propagated.empty());
    std::cout << "PASSED (Generated " << propagated.size() << " wave vertices)\n";

    // Test 4: Manifest loading from Orchestrator (modpack.json)
    std::cout << "[Test 4] Loading Orchestrator modpack.json manifest: ";
    mgr.unload_all();
    assert(mgr.get_active_overhang_strategy() == nullptr);

    fs::path tmp_manifest = fs::temp_directory_path() / "test_frankenstein_modpack.json";
    {
        std::ofstream f(tmp_manifest);
        f << R"({
            "modpack_id": "frankenstein-wave",
            "name": "Orca Frankenstein (Wave Edition)",
            "mods": [
                {
                    "id": "wave_overhangs",
                    "name": "Wave Overhangs (Andersons/Klappe)",
                    "replaces": "OverhangStrategy",
                    "enabled": true
                }
            ]
        })";
    }

    bool loaded = mgr.load_modpack(tmp_manifest);
    assert(loaded);
    assert(mgr.has_mod("wave_overhangs"));
    assert(mgr.get_active_overhang_strategy() != nullptr);
    assert(mgr.get_active_overhang_strategy()->name() == "Wave Overhangs (Dennis Klappe / Janis Andersons)");
    fs::remove(tmp_manifest);
    std::cout << "PASSED (Manifest successfully parsed and mod hooks injected!)\n";

    std::cout << "\n=================================================================\n";
    std::cout << "  ALL MODULAR SUBSYSTEM & WAVE OVERHANGS VERIFICATIONS PASSED!\n";
    std::cout << "=================================================================\n";
    return 0;
}
