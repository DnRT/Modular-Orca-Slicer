#pragma once

// OrcaSlicer C++23 Compatibility Layer: mdspan
// Bridges std::mdspan (native C++23) with kokkos::mdspan (C++20 polyfill)

#if defined(__cpp_lib_mdspan) && (__cpp_lib_mdspan >= 202207L)
    #include <mdspan>
    namespace orca::compat {
        template <class ElementType, class Extents, class LayoutPolicy = std::layout_right, class AccessorPolicy = std::default_accessor<ElementType>>
        using mdspan = std::mdspan<ElementType, Extents, LayoutPolicy, AccessorPolicy>;

        template <class IndexType, size_t... Extents>
        using extents = std::extents<IndexType, Extents...>;

        inline constexpr size_t dynamic_extent = std::dynamic_extent;
        using layout_right = std::layout_right;
        using layout_left = std::layout_left;
    }
#else
    #include "detail/kokkos_mdspan.hpp"
    namespace orca::compat {
        template <class ElementType, class Extents, class LayoutPolicy = Kokkos::layout_right, class AccessorPolicy = Kokkos::default_accessor<ElementType>>
        using mdspan = Kokkos::mdspan<ElementType, Extents, LayoutPolicy, AccessorPolicy>;

        template <class IndexType, size_t... Extents>
        using extents = Kokkos::extents<IndexType, Extents...>;

        inline constexpr size_t dynamic_extent = Kokkos::dynamic_extent;
        using layout_right = Kokkos::layout_right;
        using layout_left = Kokkos::layout_left;
    }
#endif
