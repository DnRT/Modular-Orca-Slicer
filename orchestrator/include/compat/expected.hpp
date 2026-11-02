#pragma once

// OrcaSlicer C++23 Compatibility Layer: expected
// Bridges std::expected (native C++23) with tl::expected (C++20 polyfill)

#if defined(__cpp_lib_expected) && (__cpp_lib_expected >= 202202L)
    #include <expected>
    namespace orca::compat {
        template <typename T, typename E>
        using expected = std::expected<T, E>;

        template <typename E>
        using unexpected = std::unexpected<E>;

        using unexpect_t = std::unexpect_t;
        inline constexpr std::unexpect_t unexpect{};
        using bad_expected_access = std::bad_expected_access<void>;
    }
#else
    #include "detail/tl_expected.hpp"
    namespace orca::compat {
        template <typename T, typename E>
        using expected = tl::expected<T, E>;

        template <typename E>
        using unexpected = tl::unexpected<E>;

        using unexpect_t = tl::unexpect_t;
        inline constexpr tl::unexpect_t unexpect{};
    }
#endif
