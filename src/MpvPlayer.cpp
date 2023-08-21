//
// Created by phwhitfield on 27.04.23.
//


#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "MpvPlayer.h"

#include <mpv/client.h>
#include <mpv/render.h>

using namespace godot;

void MpvPlayer::load() {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    const auto source_resolved = ProjectSettings::get_singleton()->globalize_path(source);
    if (source_loaded == source_resolved) {
        return;
    }
    source_loaded = source_resolved;
    run_thread(source_loaded);
}

void MpvPlayer::play() {
    load();
}

void MpvPlayer::pause() {

}

void MpvPlayer::seek(float seconds) {

}

void MpvPlayer::stop() {

}

void MpvPlayer::stop_thread() {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    request_exit = true;
    if (thread.joinable()) {
        print_line("thread request stop");
        thread.join();
        print_line("thread stopped");
    }
}


void MpvPlayer::_notification(int p_what) {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    switch (p_what) {
        case NOTIFICATION_READY:
            image = Ref<Image>(memnew(Image));
            image_thread = Ref<Image>(memnew(Image));
            texture = Ref<ImageTexture>(memnew(ImageTexture));
            set_process(true);
            break;
        case NOTIFICATION_PROCESS:
            process();
            break;
        case NOTIFICATION_EXIT_TREE:
            stop_thread();
            break;
    }
}


void MpvPlayer::process() {
    const auto prev_state = playback_state;
    mutex.lock();
    playback_state = playback_state_thread;
    mutex.unlock();


    if (is_frame_new) {

        is_frame_new = false;

        mutex.lock();
        image->copy_from(image_thread);
        mutex.unlock();

        if (texture->get_width() != image->get_width() || texture->get_height() != image->get_height() ||
            texture->get_format() != image->get_format()) {
            texture->set_image(image);
        } else {
            texture->update(image);
        }

    }

    // do some checks here
    if (playback_state.playing) {
        if (!prev_state.playing) {
            emit_signal("playback_started");
        }
    } else {
        if (prev_state.playing) {
            emit_signal("playback_stopped");
        }
    }
}

void MpvPlayer::run_thread(const String &source) {
    stop_thread();

    request_exit = false;


    thread = std::thread([&] {

        print_line("thread started");

        bool request_render = false;

        PlaybackState state;

        PackedByteArray pixels;

        auto check_error([&](int status) {
            state.error = false;
            if (status < 0) {
                print_error("mpv error: " + String(mpv_error_string(status)));
                state.error = true;
            }
            return state.error;
        });

        mpv_render_context *mpv_rd = nullptr;
        auto mpv = mpv_create();
        if (!mpv) {
            print_error("could not create mpv instance");
            return;
        }
        if (check_error(mpv_initialize(mpv))) {
            print_error("could not initialize mpv instance");
            return;
        }


        int int_1 = 1;
        mpv_render_param params[] = {
                {MPV_RENDER_PARAM_API_TYPE,         (void *) MPV_RENDER_API_TYPE_SW},
                // Tell libmpv that you will call mpv_render_context_update() on render
                // context update callbacks, and that you will _not_ block on the core
                // ever (see <libmpv/render.h> "Threading" section for what libmpv
                // functions you can call at all when this is active).
                // In particular, this means you must call e.g. mpv_command_async()
                // instead of mpv_command().
                // If you want to use synchronous calls, either make them on a separate
                // thread, or remove the option below (this will disable features like
                // DR and is not recommended anyway).
                {MPV_RENDER_PARAM_ADVANCED_CONTROL, &int_1},
                {MPV_RENDER_PARAM_INVALID,          nullptr}
        };

        if (check_error(mpv_render_context_create(&mpv_rd, mpv, params))) {
            print_error("[MPV] failed to initialize render context");
            return;
        }

        mpv_render_context_set_update_callback(mpv_rd, [](void *ctx) {
            auto request_render = static_cast<bool *>(ctx);
            *request_render = true;
        }, &request_render);


        // see available properties https://mpv.io/manual/master/#properties
        mpv_observe_property(mpv, 0, "dwidth", MPV_FORMAT_INT64);
        mpv_observe_property(mpv, 0, "dheight", MPV_FORMAT_INT64);

        mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
        mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
        mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
        mpv_observe_property(mpv, 0, "percent-pos", MPV_FORMAT_DOUBLE);
        mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);


        auto send_command = [&](std::vector<const char *> cmd) {
            check_error(mpv_command(mpv, cmd.data()));
        };

        send_command({"loadfile", source.utf8().get_data(), nullptr});


//        if (loop) {
//            mpv_set_option_string(mpv, "loop", "inf");
//        }

        while (!request_exit) {
            auto evt = mpv_wait_event(mpv, .5);
//        if (evt->event_id == MPV_EVENT_NONE) {
//            end();
//            break;
//        }

//        if(evt->event_id != MPV_EVENT_NONE){
//            std::cout << evt->event_id << std::endl;
//        }


            switch (evt->event_id) {
                case MPV_EVENT_NONE:
                    break;
                case MPV_EVENT_SHUTDOWN:
                    break;
                case MPV_EVENT_LOG_MESSAGE:
                    break;
                case MPV_EVENT_GET_PROPERTY_REPLY:
                    break;
                case MPV_EVENT_SET_PROPERTY_REPLY:
                    break;
                case MPV_EVENT_COMMAND_REPLY:
                    break;
                case MPV_EVENT_START_FILE:
                    print_line("[MPV] start");
                    state.playing = true;
                    //emit_signal("play");
//                    mtx.lock();
//                    event_queue.emplace_back("play");
//                    mtx.unlock();
                    break;
                case MPV_EVENT_END_FILE: {
                    print_line("[MPV] stop");

//                if (loop) {
//                    print_line("start loop");
//                    set_position_percent(0);
//                    break;
//                }

                    state.playing = false;
//                emit_signal("stop");
//                    // todo handle with mpv directly
//                    mtx.lock();
//                    event_queue.emplace_back("stop");
//                    mtx.unlock();
////
                }
                    break;
                case MPV_EVENT_FILE_LOADED:
                    print_line("file loaded");
                    break;
                case MPV_EVENT_CLIENT_MESSAGE:
                    break;
                case MPV_EVENT_VIDEO_RECONFIG:
//                std::cout << "RECONFIG" << std::endl;
                    break;
                case MPV_EVENT_AUDIO_RECONFIG:
                    break;
                case MPV_EVENT_SEEK:
                    break;
                case MPV_EVENT_PLAYBACK_RESTART:
                    break;
                case MPV_EVENT_PROPERTY_CHANGE: {
                    auto evt_prop = reinterpret_cast<mpv_event_property *>(evt->data);
                    const std::string property = evt_prop->name;

                    auto data_double = [&]() -> double {
                        if (evt_prop->format == MPV_FORMAT_DOUBLE) {
                            return *reinterpret_cast<double *>(evt_prop->data);
                        }
                        return 0;
                    };

                    auto data_bool = [&]() -> bool {
                        if (evt_prop->format == MPV_FORMAT_FLAG) {
                            return *reinterpret_cast<int *>(evt_prop->data) == 1;
                        }
                        return false;
                    };

                    auto data_int = [&]() -> int {
                        if (evt_prop->format == MPV_FORMAT_NONE) {
                            return 0;
                        }
                        if (evt_prop->format == MPV_FORMAT_INT64) {
                            return *reinterpret_cast<int64_t *>(evt_prop->data);
                        }
                        print_error(property.c_str(), ": invalid int format - ", evt_prop->format);
                        return 0;
                    };

                    if (property == "time-pos") {
                        state.position_seconds = data_double();
                    } else if (property == "percent-pos") {
                        state.position_percent = data_double() / 100.;
                    } else if (property == "duration") {
                        state.duration = data_double();
                    } else if (property == "pause") {
                        state.paused = data_bool();
                    } else if (property == "dwidth") {
                        state.width = data_int();
                        print_line("width: ", state.width);
//                        allocate_pixels();
                    } else if (property == "dheight") {
                        state.height = data_int();
                        print_line("height: ", state.height);
//                        allocate_pixels();
                    }
                }
                    break;
                case MPV_EVENT_QUEUE_OVERFLOW:
                    print_line("overflow");
                    break;
                case MPV_EVENT_HOOK:
                    break;
            }

            // check if we have a new frame
            if (request_render) {
                auto flags = mpv_render_context_update(mpv_rd);
                if (flags & MPV_RENDER_UPDATE_FRAME && state.width > 0 && state.height > 0) {
                    // get the frame
//                auto allocate_pixels = [&] {
//                    const auto width = state.width;
//                    const auto height = state.height;
//                    const auto size = width * height * 4;
//                    pixels.resize(size);
//                };

                    const auto buffer_size = state.width * state.height * 4;
                    if (pixels.size() != buffer_size) {
                        pixels.resize(buffer_size);
                    }

                    const char *format = "rgb0";
                    Vector2i size = {state.width, state.height};
                    size_t stride = state.width * 4;

                    auto pixels_write = pixels.ptrw();

                    mpv_render_param params[] = {
                            {MPV_RENDER_PARAM_SW_SIZE,    &size},
                            {MPV_RENDER_PARAM_SW_FORMAT,  (void *) format},
                            {MPV_RENDER_PARAM_SW_STRIDE,  &stride},
                            {MPV_RENDER_PARAM_SW_POINTER, pixels_write},
                            {MPV_RENDER_PARAM_INVALID,    nullptr}
                    };
                    if (check_error(mpv_render_context_render(mpv_rd, params))) {
                        print_error("error render context");
                        print_error("size", size);
                        print_error("stride", stride);
                        print_error("format", format);
                        print_error("pixels", pixels.size());
                        continue;
                    }

                    // set alpha
                    for (size_t i = 3; i < pixels.size(); i += 4) {
                        pixels_write[i] = 255;
                    }

                    // pass frame to main thread
                    mutex.lock();
                    image_thread->set_data(state.width, state.height, false, Image::FORMAT_RGBA8, pixels);
                    mutex.unlock();
                    is_frame_new = true;
                }
            }


            // pass state to main thread
            mutex.lock();
            playback_state_thread = state;
            mutex.unlock();

        }

        mpv_render_context_free(mpv_rd);
        mpv_terminate_destroy(mpv);
    });

    //    print_line("starting thread");
}

void MpvPlayer::set_source(const String &s) {
    source = s;
    if (autoplay) {
        play();
    }
}

const String &MpvPlayer::get_source() const {
    return source;
}

void MpvPlayer::set_autoplay(bool value) {
    autoplay = value;
}

bool MpvPlayer::get_autoplay() const {
    return autoplay;
}

Ref<ImageTexture> MpvPlayer::get_texture() const {
    return texture;
}

// bindings
void MpvPlayer::_bind_methods() {
    ADD_SIGNAL(MethodInfo("playback_started"));
    ADD_SIGNAL(MethodInfo("playback_stopped"));

    ClassDB::bind_method(D_METHOD("set_source", "source"), &MpvPlayer::set_source);
    ClassDB::bind_method(D_METHOD("get_source"), &MpvPlayer::get_source);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "source"), "set_source", "get_source");

    ClassDB::bind_method(D_METHOD("set_autoplay", "autoplay"), &MpvPlayer::set_autoplay);
    ClassDB::bind_method(D_METHOD("get_autoplay"), &MpvPlayer::get_autoplay);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

    ClassDB::bind_method(D_METHOD("play"), &MpvPlayer::play);
//    ClassDB::bind_method(D_METHOD("pause"), &MpvPlayer::pause);
    ClassDB::bind_method(D_METHOD("get_texture"), &MpvPlayer::get_texture);
//    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "ImageTexture"),
//                 "", "get_texture");
}

