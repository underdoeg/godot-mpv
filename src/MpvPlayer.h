#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <thread>

namespace godot {

//
//    class VideoPlayer: public Resource {
//    };

    class MpvPlayer : public Node {
    GDCLASS(MpvPlayer, Node)

    private:
        Ref<ImageTexture> texture;

        std::thread thread;
        std::atomic_bool keep_running{false};
        void stop_thread();
        void run_thread();

    protected:
        static void _bind_methods();

    public:
        void load(const String &file_path);

        void play();

        void pause();

        void seek(float seconds);

        void stop();
    };

}