cmake_minimum_required(VERSION 3.16)
project(OrcaOrchestrator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/compat
)

# 1. Native GUI Launcher (Zero-console turnkey maker entry point)
add_executable(OrcaSlicerLauncher WIN32 Launcher.cpp)
if (WIN32)
    target_link_libraries(OrcaSlicerLauncher PRIVATE Shell32 User32 Ole32)
endif ()

# 2. CLI Modpack Orchestrator (For advanced maker workflows & automation)
set(ORCHESTRATOR_SOURCES
    main.cpp
    ProcessManager.cpp
    ProcessManager.hpp
    FrankensteinProfile.cpp
    FrankensteinProfile.hpp
)

add_executable(OrcaOrchestrator ${ORCHESTRATOR_SOURCES})

if (WIN32)
    target_link_libraries(OrcaOrchestrator PRIVATE Shell32 User32)
endif ()
