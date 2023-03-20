#ifndef MODULES_H
#define MODULES_H

#include <stdint.h>
#include "engine.h"

// Unscriptable API functions
renderer* renderer_alloc();
void renderer_free(renderer*);
void initglew();
void draw(renderer*);

texture addtex(renderer*, const uint8_t* buf, unsigned w, unsigned h);

sprite addsprite(renderer*, uint16_t numquads);
void deletesprite(renderer*, sprite);
void set_cam(renderer*, float cam_x, float cam_y);
void set_res(renderer*, uint16_t res_x, uint16_t res_y);


audio* audio_alloc();
void audio_free(audio*);

enum class output_mode {
    mono = 1, stereo
};



void initialize(audio*, int sample_rate, output_mode nc);
void load_from_file(audio*, const char* filename, sink_channel channel);
void mix_audio(audio*, uint32_t num_samples);



struct window_event {
	enum class type {
	    null, quit, resize, keyevent, mousebutton, cursor
	};
    type event;
    uint32_t data[8]; // 32 bytes of data buffer
};


window* window_alloc();
void window_free(window*);

void set_vsync(bool state);
void swap_buffers(window*);
void set_resolution(window*, uint16_t res_w, uint16_t rew_h);
int poll_events(window*, window_event* wev);

#endif //MODULES_H
