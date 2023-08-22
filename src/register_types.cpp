#include "register_types.h"
#include "MpvPlayer.h"
#include "MpvTextureRect.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_mpv_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        ClassDB::register_class<MpvPlayer>();
        ClassDB::register_class<MpvTextureRect>();
    }
}

void uninitialize_mpv_module(ModuleInitializationLevel p_level) {

}

// uncomment in next godot4 beta, gdnative was renamed to gdextension
extern "C"
{
GDExtensionBool GDE_EXPORT
godot_mpv_library_init(const GDExtensionInterfaceGetProcAddress p_interface, const GDExtensionClassLibraryPtr p_library,
                GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

    init_obj.register_initializer(initialize_mpv_module);
    init_obj.register_terminator(uninitialize_mpv_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
