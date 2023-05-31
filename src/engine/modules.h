#ifndef MODULES_H
#define MODULES_H

#include <stdint.h>
#include "engine.h"

class renderer;
class audio;
class window;

// Rendering functions
renderer* renderer_alloc();
void renderer_free(renderer*);
void initglew();
void draw(renderer*);

texture addtex(renderer*, const char* filename);
texture addtex(renderer*, const uint8_t* buf, unsigned w, unsigned h);
sprite addsprite(renderer*, uint16_t numquads);
void deletesprite(renderer*, sprite);
void set_cam(renderer*, float cam_x, float cam_y);
void set_res(renderer*, uint16_t res_x, uint16_t res_y);
void apply_tf(renderer*, sprite, float* mat);
void get_origin(renderer*, sprite, uint16_t* x, uint16_t* y);
void settex(renderer*, sprite, texture);
void setbounds(renderer*, sprite, uint16_t quad, int x, int y, int w, int h);
void setuv(renderer*, sprite, uint16_t quad, float tlx, float tly, float w, float h);


// Window Management Functions
window* window_alloc();
void window_free(window*);

struct window_event {
	enum class type {
	    null, quit, resize, keyevent, mousebutton, cursor
	} event;
    uint32_t data[8]; // 32 bytes of data buffer
};

void set_vsync(bool state);
void swap_buffers(window*);
void set_resolution(window*, uint16_t res_w, uint16_t rew_h);
int poll_events(window*, window_event* wev);


// Audio Player Functions
audio* audio_alloc();
void audio_free(audio*);

using trackid = uint16_t;
using stream_id = uint8_t;

enum class output_mode { mono = 1, stereo };
enum class sink_channel { music, effects, master };

void initialize(audio*, int sample_rate, output_mode nc);
void load_from_file(audio*, const char* filename, sink_channel channel);
void mix_audio(audio*, uint32_t num_samples);
stream_id play_track(audio*, uint32_t track_id);
void stop_stream(audio*, stream_id track);
void set_channel_volume(audio*, sink_channel channel, float volume);


#endif //MODULES_H
