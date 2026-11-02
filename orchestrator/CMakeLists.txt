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

# Source files
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
