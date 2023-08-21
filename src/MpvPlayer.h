#pragma once

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/core/memory.hpp>
#include <thread>
#include <mutex>

namespace godot {

    class MpvPlayer : public Node {
    GDCLASS(MpvPlayer, Node)

    private:

        struct PlaybackState {
            bool playing = false;
            bool paused = false;
            bool stopped = false;
            bool ended = false;
            bool seeking = false;
            bool buffering = false;
            bool error = false;
            int width = 0;
            int height = 0;
            double duration = 0;
            double position_seconds = 0;
            double position_percent = 0;
        };

        PlaybackState playback_state;
        PlaybackState playback_state_thread;

        Ref<Image> image;
        Ref<Image> image_thread;

        std::thread thread;
        std::mutex mutex;
        std::atomic_bool request_exit{false};
        std::atomic_bool is_frame_new{false};

        Ref<ImageTexture> texture;
        String source;
        String source_loaded;

        bool autoplay;

        void stop_thread();

        void run_thread(const String &source);

        template<class... Args>
        void print_line(const Variant &arg1, const Args &... args) {
            UtilityFunctions::print("[MPV - ", source, "] ", arg1, args...);
        }

        template<class... Args>
        void print_error(const Variant &arg1, const Args &... args) {
            UtilityFunctions::printerr("[MPV - ", source, "] ", arg1, args...);
        }

    protected:
        static void _bind_methods();

        void _notification(int p_what);

        void load();
        void process();

    public:

        void set_source(const String &source);

        const String &get_source() const;

        Ref<ImageTexture> get_texture() const;

        void set_autoplay(bool value);

        bool get_autoplay() const;

        void play();

        void pause();

        void seek(float seconds);

        void stop();

    };

}