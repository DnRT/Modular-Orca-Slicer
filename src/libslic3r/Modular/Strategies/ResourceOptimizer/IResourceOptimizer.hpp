#pragma once

#include <string>
#include <cstddef>

namespace Slic3r::Modular {

// Resource Optimizer Interface: memory bounding and CPU scheduling policy
class IResourceOptimizer {
public:
    virtual ~IResourceOptimizer() = default;

    virtual std::string id() const = 0;
    virtual std::string name() const = 0;

    // Checks if the optimizer is active
    virtual bool is_active() const = 0;

    // Calculates concurrency limit (threads) based on triangle count and system resources
    virtual size_t get_concurrency_ceiling(size_t default_threads, size_t triangle_count) = 0;

    // Determines if memory-intensive intermediate slice buffers should be eagerly released
    virtual bool should_eagerly_release_buffers(size_t triangle_count) = 0;

    // Lifecycle hooks
    virtual void on_slicing_started(size_t triangle_count) = 0;
    virtual void on_slicing_completed() = 0;
};

} // namespace Slic3r::Modular
