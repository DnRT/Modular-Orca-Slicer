#pragma once

#include <string>
#include <memory>
#include <algorithm>
#include "libslic3r/Modular/ModuleABI.hpp"
#include "libslic3r/Modular/Strategies/ResourceOptimizer/IResourceOptimizer.hpp"

namespace Slic3r::Modular::Templates {

/**
 * @brief Template Plugin: Custom Resource & Concurrency Optimizer
 * 
 * Developers can inherit from IOrcaMod and IResourceOptimizer to regulate
 * CPU thread concurrency, memory footprint, and eager garbage collection
 * during slicing operations on dense meshes.
 */
class TemplateResourceOptimizerPlugin : public IOrcaMod, public IResourceOptimizer {
public:
    TemplateResourceOptimizerPlugin() = default;
    virtual ~TemplateResourceOptimizerPlugin() = default;

    // --- IOrcaMod Lifecycle & Identity Metadata ---
    const char* get_id() const override { return "template_resource_optimizer"; }
    const char* get_name() const override { return "Adaptive Dynamic Memory Scheduler"; }
    const char* get_version() const override { return "1.0.0"; }
    const char* get_author() const override { return "Community Developer"; }
    const char* get_description() const override {
        return "Template demonstrating thread capping and eager memory reclamation for low-RAM machines.";
    }

    SubsystemHook get_target_hook() const override {
        return SubsystemHook::ResourceOptimizer;
    }

    bool on_load(ModuleContext &ctx) override {
        m_active = true;
        ctx.log_info("TemplateResourceOptimizerPlugin: Initialized successfully.");
        return true;
    }

    void on_unload() override {
        m_active = false;
    }

    // --- IResourceOptimizer Implementation ---
    std::string id() const override { return get_id(); }
    std::string name() const override { return get_name(); }

    bool is_active() const override {
        return m_active;
    }

    size_t get_concurrency_ceiling(size_t default_threads, size_t triangle_count) override {
        if (!m_active) return default_threads;

        // Custom heuristic: if triangle count exceeds 1 million, limit concurrency to prevent OOM
        if (triangle_count > 1000000) {
            return std::min<size_t>(default_threads, 4);
        }
        return default_threads;
    }

    bool should_eagerly_release_buffers(size_t triangle_count) override {
        if (!m_active) return false;
        // Release intermediate raw slices eagerly on complex models
        return triangle_count > 500000;
    }

    void on_slicing_started(size_t triangle_count) override {
        // Prepare pre-allocations or profiling timers
    }

    void on_slicing_completed() override {
        // Collect metrics or clean temporary pools
    }

private:
    bool m_active{true};
};

} // namespace Slic3r::Modular::Templates
