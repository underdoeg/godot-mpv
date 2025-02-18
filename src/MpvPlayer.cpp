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

    if (source.is_empty()) {
        return;
    }

    const auto source_resolved = ProjectSettings::get_singleton()->globalize_path(source);
    String subtitle_source_resolved = "";
    if (!subtitle_source.is_empty()) {
        subtitle_source_resolved = ProjectSettings::get_singleton()->globalize_path(subtitle_source);
    }
    if (source_loaded == source_resolved && subtitle_source_resolved == subtitle_source_loaded) {
        return;
    }
    source_loaded = source_resolved;
    subtitle_source_loaded = subtitle_source_resolved;
    run_thread(source_loaded, subtitle_source_loaded);
}


void MpvPlayer::add_cmd(const mpv_cmd::Cmd &cmd) {
    std::scoped_lock g(mutex);
    cmd_queue.push_back(cmd);
}

std::vector<mpv_cmd::Cmd> MpvPlayer::clear_cmd_queue() {
    std::scoped_lock g(mutex);
    auto copy = cmd_queue;
    cmd_queue.clear();
    return std::move(copy);
}

void MpvPlayer::set_source(const String &s) {
    source = s;
    if (autoplay) {
        load();
    }
}

const String &MpvPlayer::get_source() const {
    return source;
}

void MpvPlayer::set_subtitle_source(const String &s) {
    subtitle_source = s;
    if (is_playing()) {
        load();
    }
}

const String &MpvPlayer::get_subtitle_source() const {
    return subtitle_source;
}

void MpvPlayer::set_autoplay(bool value) {
    autoplay = value;
    if (autoplay) {
        load();
    }
}

bool MpvPlayer::get_autoplay() const {
    return autoplay;
}

Ref<ImageTexture> MpvPlayer::get_texture() const {
    return texture;
}

float MpvPlayer::get_duration() const {
    return playback_state.duration;
}

void MpvPlayer::play() {
    load();
    add_cmd(mpv_cmd::Play());
}

void MpvPlayer::pause() {
    add_cmd(mpv_cmd::Pause());
}

bool MpvPlayer::is_paused() const {
    return playback_state.paused;
}

bool MpvPlayer::is_stopped() const {
    return !playback_state.playing;
}

bool MpvPlayer::is_playing() const {
    return playback_state.playing;
}

void MpvPlayer::set_paused(bool paused) {
    if (paused) {
        pause();
    } else {
        play();
    }
}

void MpvPlayer::seek_seconds(float seconds) {
    add_cmd(mpv_cmd::SeekSeconds{seconds});
}


void MpvPlayer::seek_percent(float percent) {
    add_cmd(mpv_cmd::SeekPercent{percent});
}

void MpvPlayer::stop() {
    add_cmd(mpv_cmd::Stop());
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

        emit_signal("frame_updated");
    }

    // do some checks here
    if (playback_state.playing) {
        if (!prev_state.playing) {
            emit_signal("playback_started");
        }
    } else {
        if (prev_state.playing) {
            source_loaded = "";
            emit_signal("playback_stopped");
        }
    }
}

void MpvPlayer::run_thread(const String &source, const String &subtitle_source) {
    stop_thread();

    request_exit = false;


    thread = std::thread([&] {
        print_line("thread started");

        bool request_render = false;
        bool first_frame_rendered = false;

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
        int sw_size[2] = {1920, 1080};
        // const char *sw_format = "rgb0";
        // int sw_stride = 1920 * 4;

        auto render_type = (void *) MPV_RENDER_API_TYPE_SW;
        // char* render_type = "sw";
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, render_type},
            // {MPV_RENDER_PARAM_API_TYPE,         (void *) MPV_RENDER_API_TYPE_SW},
            // Tell libmpv that you will call mpv_render_context_update() on render
            // context update callbacks, and that you will _not_ block on the core
            // ever (see <libmpv/render.h> "Threading" section for what libmpv
            // functions you can call at all when this is active).
            // In particular, this means you must call e.g. mpv_command_async()
            // instead of mpv_command().
            // If you want to use synchronous calls, either make them on a separate
            // thread, or remove the option below (this will disable features like
            // DR and is not recommended anyway).
            //  MPV_RENDER_PARAM_SW_SIZE, MPV_RENDER_PARAM_SW_FORMAT,
            {MPV_RENDER_PARAM_SW_SIZE, sw_size},
            {MPV_RENDER_PARAM_ADVANCED_CONTROL, &int_1},
            {MPV_RENDER_PARAM_INVALID, nullptr}
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


        if (!subtitle_source.is_empty()) {
            check_error(mpv_set_property_string(mpv, "sub-files", subtitle_source.utf8().get_data()));
        }

        auto send_command = [&](std::vector<const char *> cmd) {
            check_error(mpv_command(mpv, cmd.data()));
        };

        send_command({
            "loadfile", source.utf8().get_data(),
            nullptr
        });
        //, "sub-file", "/home/phwhitfield/archive/transmission/Color.Out.of.Space.2019.1080p.SCREENER.x264-Rapta.srt", nullptr});


        //        if (loop) {
        //            mpv_set_option_string(mpv, "loop", "inf");
        //        }

        while (!request_exit) {
            // handle commands
            auto cmds = clear_cmd_queue();
            for (const auto cmd: cmds) {
                std::visit([&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, mpv_cmd::Play>) {
                        send_command({"set", "pause", "no", nullptr});
                    } else if constexpr (std::is_same_v<T, mpv_cmd::Pause>) {
                        send_command({"set", "pause", "yes", nullptr});
                    } else if constexpr (std::is_same_v<T, mpv_cmd::Stop>) {
                        send_command({"stop", nullptr});
                    } else if constexpr (std::is_same_v<T, mpv_cmd::SeekSeconds>) {
                        send_command({"seek", std::to_string(arg.seconds).c_str(), "absolute", nullptr});
                    } else if constexpr (std::is_same_v<T, mpv_cmd::SeekPercent>) {
                        send_command({"seek", std::to_string(arg.percent).c_str(), "absolute-percent", nullptr});
                    }
                }, cmd);
            }


            // check for mpv events
            auto evt = mpv_wait_event(mpv, .5);

            switch (evt->event_id) {
                case MPV_EVENT_NONE:
                    break;
                case MPV_EVENT_SHUTDOWN:
                    break;
                case MPV_EVENT_LOG_MESSAGE: {
                    auto evt_prop = reinterpret_cast<mpv_event_log_message *>(evt->data);
                    print_line("[MPV] ", evt_prop->prefix, ": ", evt_prop->text);
                    break;
                }
                case MPV_EVENT_GET_PROPERTY_REPLY:
                    break;
                case MPV_EVENT_SET_PROPERTY_REPLY:
                    break;
                case MPV_EVENT_COMMAND_REPLY:
                    break;
                case MPV_EVENT_START_FILE:
                    print_line("start");
                    state.playing = true;
                //emit_signal("play");
                //                    mtx.lock();
                //                    event_queue.emplace_back("play");
                //                    mtx.unlock();
                    break;
                case MPV_EVENT_END_FILE: {
                    print_line("stop");

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
                        state.position_percent = data_double();
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
                // if (flags & MPV_RENDER_UPDATE_FRAME && state.width > 0 && state.height > 0) {
                // get the frame
                // auto allocate_pixels = [&] {
                //     const auto width = state.width;
                //     const auto height = state.height;
                //     const auto size = width * height * 4;
                //     pixels.resize(size);
                // };
                print_line("frame updated", flags);
                if (flags & MPV_RENDER_UPDATE_FRAME) {
                    const auto buffer_size = state.width * state.height * 4;
                    if (pixels.size() != buffer_size) {
                        pixels.resize(buffer_size);
                    }

                    const char *format = "0bgr";
                    Vector2i size = {state.width, state.height};
                    size_t stride = state.width * 4;
                    if (size.x && size.y) {
                        auto pixels_write = pixels.ptrw();

                        mpv_render_param params[] = {
                            {MPV_RENDER_PARAM_SW_SIZE, &size},
                            {MPV_RENDER_PARAM_SW_FORMAT, (void *) format},
                            {MPV_RENDER_PARAM_SW_STRIDE, &stride},
                            {MPV_RENDER_PARAM_SW_POINTER, pixels_write},
                            {MPV_RENDER_PARAM_INVALID, nullptr}
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
                        //print_line("frame updated", state.width);

                        // pass frame to main thread
                        mutex.lock();
                        image_thread->set_data(state.width, state.height, false, Image::FORMAT_RGBA8, pixels);
                        mutex.unlock();
                        is_frame_new = true;
                        first_frame_rendered = true;
                    }
                }
            }


            // pass state to main thread
            mutex.lock();
            playback_state_thread = state;
            playback_state_thread.playing = first_frame_rendered && state.playing;
            mutex.unlock();
        }

        mpv_render_context_free(mpv_rd);
        mpv_terminate_destroy(mpv);
    });

    //    print_line("starting thread");
}


// bindings
void MpvPlayer::_bind_methods() {
    ADD_SIGNAL(MethodInfo("playback_started"));
    ADD_SIGNAL(MethodInfo("playback_stopped"));
    ADD_SIGNAL(MethodInfo("frame_updated"));

    ClassDB::bind_method(D_METHOD("set_source", "source"), &MpvPlayer::set_source);
    ClassDB::bind_method(D_METHOD("get_source"), &MpvPlayer::get_source);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "source"), "set_source", "get_source");

    ClassDB::bind_method(D_METHOD("set_subtitle_source", "source"), &MpvPlayer::set_subtitle_source);
    ClassDB::bind_method(D_METHOD("get_subtitle_source"), &MpvPlayer::get_subtitle_source);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "subtitle_source"), "set_subtitle_source", "get_subtitle_source");

    ClassDB::bind_method(D_METHOD("set_autoplay", "autoplay"), &MpvPlayer::set_autoplay);
    ClassDB::bind_method(D_METHOD("get_autoplay"), &MpvPlayer::get_autoplay);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

    ClassDB::bind_method(D_METHOD("play"), &MpvPlayer::play);
    ClassDB::bind_method(D_METHOD("pause"), &MpvPlayer::pause);
    ClassDB::bind_method(D_METHOD("stop"), &MpvPlayer::stop);
    ClassDB::bind_method(D_METHOD("get_texture"), &MpvPlayer::get_texture);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "ImageTexture",
                              PROPERTY_USAGE_NO_EDITOR), "", "get_texture");

    ClassDB::bind_method(D_METHOD("seek_seconds", "seconds"), &MpvPlayer::seek_seconds);
    ClassDB::bind_method(D_METHOD("seek_percent", "percent"), &MpvPlayer::seek_percent);
    ClassDB::bind_method(D_METHOD("get_position_seconds"), &MpvPlayer::get_position_seconds);
    ClassDB::bind_method(D_METHOD("get_position_percent"), &MpvPlayer::get_position_percent);
    ClassDB::bind_method(D_METHOD("get_duration"), &MpvPlayer::get_duration);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR),
                 "", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "position_seconds", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR),
                 "seek_seconds", "get_position_seconds");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "position_percent", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR),
                 "seek_percent", "get_position_percent");

    ClassDB::bind_method(D_METHOD("is_paused"), &MpvPlayer::is_paused);
    ClassDB::bind_method(D_METHOD("set_paused", "paused"), &MpvPlayer::set_paused);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "paused", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_paused",
                 "is_paused");
}

float MpvPlayer::get_position_seconds() const {
    return playback_state.position_seconds;
}

float MpvPlayer::get_position_percent() const {
    return playback_state.position_percent;
}
