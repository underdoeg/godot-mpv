//
// Created by phwhitfield on 22.08.23.
//

#include <godot_cpp/classes/engine.hpp>
#include "MpvTextureRect.h"

using namespace godot;

void MpvTextureRect::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_player", "player"), &MpvTextureRect::set_player);
    ClassDB::bind_method(D_METHOD("get_player"), &MpvTextureRect::get_player);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "player", PROPERTY_HINT_NODE_TYPE, "MpvPlayer"),
                 "set_player", "get_player");
}

//void MpvTextureRect::set_player_path(const NodePath &path) {
//    player_path = path;
//    if (!player_path.is_empty()) {
//        player = get_node<MpvPlayer>(path);
//        if (player) {
//            set_texture(player->get_texture());
//        } else {
//            set_texture(Ref<Texture>());
//        };
//    } else {
//
//    }
//}
//
//const NodePath &MpvTextureRect::get_player_path() const {
//    return player_path;
//}

void MpvTextureRect::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE:
//            set_player_path(player_path);
            set_player(player);
            set_process(true);
            break;
        case NOTIFICATION_PROCESS:
            if (player && player->get_texture().is_valid() && get_texture().is_null()) {
                set_texture(player->get_texture());
            }
            break;
    }
}

void MpvTextureRect::set_player(Node *node) {
    if(node == player){
        return;
    }
    player = Object::cast_to<MpvPlayer>(node);
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (player) {
        UtilityFunctions::print("[MpvTextureRect] Setting player to ", player->get_name());
        set_texture(player->get_texture());
    } else {
        set_texture(Ref<Texture>());
    }
}

Node *MpvTextureRect::get_player() {
    return player;
}
