#ifndef MODULES_H
#define MODULES_H

#include <stdint.h>
#include <vector>
#include <string>
#include <common/coordinate_types.h>

struct sprite {
    std::vector<vertex> data;
    uint32_t tex_id;
};

namespace renderer {
    struct handle;

    handle* alloc();
    void free(handle*);
    void draw(handle*);
    void glew_init();

    uint32_t add_tex(handle*, const uint8_t* buf, unsigned w, unsigned h);
    void add_sprite(handle*, const sprite& s);

    void clear_sprites(handle*);

    void set_cam(handle*, float cam_x, float cam_y);
    void set_res(handle*, uint16_t res_x, uint16_t res_y);
}

namespace audio {
    using sample = int16_t;
    using stream_id = uint8_t;

    enum class sink_channel {
        music,
        effects,
        master
    };


    enum class output_mode {
        mono = 1,
        stereo = 2
    };

    struct handle;
    handle* alloc();
    void free(handle*);

    void initialize(handle*, int sample_rate, output_mode nc);
    void load_from_file(handle*, std::string filename, sink_channel channel);
    void mix_audio(handle*, size_t num_samples);
    stream_id play_track(handle*, size_t track_id);
    void stop_stream(handle*, stream_id track);
    void set_channel_volume(handle*, sink_channel channel, float volume);
}

namespace window {

    enum class evtype {
        null, quit, resize, keyevent, mousebutton, cursor
    };
    // class for managing window events
    struct event {
        evtype type;
        uint32_t data[8]; // 32 bytes of data buffer
    };

    struct handle;

    handle* alloc();
    void free(handle*);

    void set_vsync(bool state);
    void swap_buffers(handle*);
    void set_resolution(handle*, uint16_t res_w, uint16_t rew_h);
    int poll_events(handle*, event* wev);
}

#endif //MODULES_H
