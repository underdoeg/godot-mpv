//
// Created by phwhitfield on 27.04.23.
//

#include "MpvPlayer.h"

#include <mpv/client.h>
#include <mpv/render.h>

using namespace godot;

struct PlaybackState {
    bool playing;
    bool paused;
    bool stopped;
    bool ended;
    bool seeking;
    bool buffering;
    bool error;
};

void MpvPlayer::_bind_methods() {

}

void MpvPlayer::load(const String &file_path) {

}

void MpvPlayer::play() {

}

void MpvPlayer::pause() {

}

void MpvPlayer::seek(float seconds) {

}

void MpvPlayer::stop() {

}

void MpvPlayer::stop_thread() {
//    print_line("stopping thread");
    if (thread.joinable()) {
        thread.join();
    }
}

void MpvPlayer::run_thread() {
    stop_thread();

    keep_running = true;

    thread = std::thread([&] {

        auto mpv = mpv_create();
        mpv_initialize(mpv);

        while (keep_running) {

        }

        mpv_terminate_destroy(mpv);
    });

    //    print_line("starting thread");
}


