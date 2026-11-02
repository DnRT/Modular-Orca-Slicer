#pragma once

// ============================================================================
// OrcaSlicer C++23 / C++20 Universal Compatibility Foundation
//
// This layer enables the use of C++23 patterns (expected, mdspan, flat_map)
// cleanly across MSVC, GCC, and Apple Clang, targeting C++20 as baseline.
// ============================================================================

#include <span>
#include <concepts>
#include <thread>
#include <optional>
#include <variant>
#include <string_view>
#include <vector>

#include "expected.hpp"
#include "mdspan.hpp"
#include "flat_map.hpp"

namespace orca {
    // Bring compat namespace aliases into orca scope for convenient usage
    namespace compat {
        // Concept to verify if a type is an expected-like type
        template <typename T>
        struct is_expected : std::false_type {};

        template <typename T, typename E>
        struct is_expected<expected<T, E>> : std::true_type {};

        template <typename T>
        inline constexpr bool is_expected_v = is_expected<T>::value;
    }
}
