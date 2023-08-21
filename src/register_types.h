#pragma once

//#include <godot_cpp/godot.hpp>
//
//
//void gdextension_initialize(godot::ModuleInitializationLevel p_level);
//void gdextension_terminate(godot::ModuleInitializationLevel p_level);

#include <godot_cpp/godot.hpp>

void initialize_mpv_module(godot::ModuleInitializationLevel p_level);

void uninitialize_mpv_module(godot::ModuleInitializationLevel p_level);
