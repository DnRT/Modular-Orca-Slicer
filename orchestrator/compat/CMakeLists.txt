cmake_minimum_required(VERSION 3.13)
project(orca_compat)

add_library(orca_compat INTERFACE)

target_include_directories(orca_compat INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(orca_compat INTERFACE cxx_std_20)
