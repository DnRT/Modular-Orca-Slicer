#include <catch2/catch.hpp>
#include <compat/compat.hpp>
#include <string>

TEST_CASE("C++23 Compat: expected", "[compat]") {
    SECTION("Expected with value") {
        auto divide = [](int a, int b) -> orca::compat::expected<int, std::string> {
            if (b == 0) return orca::compat::unexpected(std::string("Division by zero"));
            return a / b;
        };

        auto ok = divide(10, 2);
        REQUIRE(ok.has_value());
        REQUIRE(ok.value() == 5);
        REQUIRE(ok.value_or(0) == 5);

        auto err = divide(10, 0);
        REQUIRE_FALSE(err.has_value());
        REQUIRE(err.error() == "Division by zero");
        REQUIRE(err.value_or(-1) == -1);
    }
}

TEST_CASE("C++23 Compat: mdspan 3D indexing", "[compat]") {
    SECTION("3D view over contiguous buffer") {
        std::vector<float> data(2 * 3 * 4, 0.0f); // 2x3x4 buffer
        
        using extents_3d = orca::compat::extents<size_t, 2, 3, 4>;
        orca::compat::mdspan<float, extents_3d> grid(data.data());

        grid(1, 2, 3) = 42.5f;
        REQUIRE(grid(1, 2, 3) == 42.5f);
        REQUIRE(data[1 * (3 * 4) + 2 * 4 + 3] == 42.5f);
    }
}

TEST_CASE("C++23 Compat: flat_map associative container", "[compat]") {
    SECTION("Basic insertion and lookup") {
        orca::compat::flat_map<std::string, int> settings;
        settings["line_spacing"] = 35;
        settings["speed"] = 100;

        REQUIRE(settings.size() == 2);
        REQUIRE(settings["line_spacing"] == 35);
        REQUIRE(settings.find("non_existent") == settings.end());
    }
}
