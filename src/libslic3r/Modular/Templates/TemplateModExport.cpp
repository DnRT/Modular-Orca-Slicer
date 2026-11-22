/**
 * @file TemplateModExport.cpp
 * @brief Dynamic Shared Library (.dll / .so) Entry Point Boilerplate
 * 
 * If your plugin is built as a standalone dynamic library instead of being statically
 * linked inside OrcaSlicer, export the standard C factory symbols defined in ModuleABI.hpp.
 */

#include "TemplateOverhangPlugin.hpp"

#if defined(_WIN32)
    #define ORCA_MOD_EXPORT __declspec(dllexport)
#else
    #define ORCA_MOD_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {

ORCA_MOD_EXPORT Slic3r::Modular::IOrcaMod* create_orca_mod() {
    return new Slic3r::Modular::Templates::TemplateOverhangPlugin();
}

ORCA_MOD_EXPORT void destroy_orca_mod(Slic3r::Modular::IOrcaMod* mod) {
    delete mod;
}

}
