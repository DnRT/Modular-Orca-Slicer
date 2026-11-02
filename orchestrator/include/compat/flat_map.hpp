#pragma once

// OrcaSlicer C++23 Compatibility Layer: flat_map & flat_set
// Bridges std::flat_map (C++23) with boost::container::flat_map (Boost available in Orca)

#if defined(__cpp_lib_flat_map) && (__cpp_lib_flat_map >= 202207L)
    #include <flat_map>
    namespace orca::compat {
        template <class Key, class T, class Compare = std::less<Key>>
        using flat_map = std::flat_map<Key, T, Compare>;

        template <class Key, class Compare = std::less<Key>>
        using flat_set = std::flat_set<Key, Compare>;
    }
#else
    #include <boost/container/flat_map.hpp>
    #include <boost/container/flat_set.hpp>
    namespace orca::compat {
        template <class Key, class T, class Compare = std::less<Key>>
        using flat_map = boost::container::flat_map<Key, T, Compare>;

        template <class Key, class Compare = std::less<Key>>
        using flat_set = boost::container::flat_set<Key, Compare>;
    }
#endif
