#pragma once

#include <godot_cpp/classes/texture_rect.hpp>
#include "MpvPlayer.h"


namespace godot {
    class MpvTextureRect : public TextureRect {

    GDCLASS(MpvTextureRect, TextureRect);

//        NodePath player_path;
        MpvPlayer *player;

    protected:
        static void _bind_methods();

        void _notification(int p_what);

    public:
        void set_player(Node *player);
        Node* get_player();
//
//        void set_player_path(const NodePath &path);
//
//        const NodePath &get_player_path() const;

    };
}

