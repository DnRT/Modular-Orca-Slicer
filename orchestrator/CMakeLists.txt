cmake_minimum_required(VERSION 3.16)
project(OrcaOrchestrator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/compat
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Source files
set(ORCHESTRATOR_SOURCES
    src/main.cpp
    src/ProcessManager.cpp
    src/ProcessManager.hpp
    src/FrankensteinProfile.cpp
    src/FrankensteinProfile.hpp
)

add_executable(OrcaOrchestrator ${ORCHESTRATOR_SOURCES})

# Windows specific flags
if (WIN32)
    # Link with shell and user32 if needed
    target_link_libraries(OrcaOrchestrator PRIVATE Shell32.lib User32.lib)
endif ()
