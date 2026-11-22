#pragma once

#include <iostream>
#include <algorithm>
#include "libslic3r/Modular/ModuleABI.hpp"
#include "IResourceOptimizer.hpp"

namespace Slic3r::Modular {

// Community Module: Large Model Resource & Memory Optimizer
// Decouples geometry chunks and throttles concurrent mesh decimation on RAM-limited hardware.
class ResourceOptimizerMod : public IOrcaMod, public IResourceOptimizer {
public:
    ResourceOptimizerMod() = default;
    virtual ~ResourceOptimizerMod() = default;

    // IOrcaMod
    const char* get_id() const override { return "resource_optimizer_lite"; }
    const char* get_name() const override { return "Low-RAM Large Model Resource Optimizer"; }
    const char* get_version() const override { return "1.1.0"; }
    const char* get_author() const override { return "Modular RepRap Community"; }
    const char* get_description() const override {
        return "Decouples mesh slicing buffers and enforces thread pool bounds for huge models on modest PCs.";
    }

    SubsystemHook get_target_hook() const override {
        return SubsystemHook::ResourceOptimizer;
    }

    bool on_load(ModuleContext &ctx) override {
        m_active = true;
        ctx.log_info("Resource Optimizer Active: Low-RAM slicing ceiling enabled.");
        return true;
    }

    void on_unload() override {
        m_active = false;
    }

    // IResourceOptimizer
    std::string id() const override { return get_id(); }
    std::string name() const override { return get_name(); }
    bool is_active() const override { return m_active; }

    size_t get_concurrency_ceiling(size_t default_threads, size_t triangle_count) override {
        if (!m_active) return default_threads;
        // If mesh is massive (> 250k triangles), cap concurrent parallel slice allocations
        // to avoid exceeding available physical RAM and triggering OS disk thrashing/paging.
        if (triangle_count > 250000) {
            return std::clamp<size_t>(default_threads, 1, 4);
        }
        return default_threads;
    }

    bool should_eagerly_release_buffers(size_t triangle_count) override {
        if (!m_active) return false;
        return triangle_count > 50000;
    }

    void on_slicing_started(size_t triangle_count) override {
        if (m_active && triangle_count > 100000) {
            std::cout << "[ResourceOptimizer] Large mesh detected (" << triangle_count 
                      << " triangles). Enforcing memory-constrained slicing profile." << std::endl;
        }
    }

    void on_slicing_completed() override {
        // Opportunity for memory trim / release
    }

private:
    bool m_active{false};
};

} // namespace Slic3r::Modular
