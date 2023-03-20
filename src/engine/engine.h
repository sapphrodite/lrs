#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include "typedefs.h"

class engine;
class renderer;
class audio;
class window;

using entity = uint16_t;
using sprite = uint16_t;
using texture = uint16_t;
using trackid = uint16_t;
using stream_id = uint8_t;


enum class sink_channel {
    music, effects, master
};

renderer* get_renderer(engine*);
audio* get_audio(engine*);

engine* alloc_engine();
void free_engine(engine* e);
bool poll_events(engine*);



void run_tick(engine*);
void render(engine* e);

entity addentity(engine*);
sprite addsprite(engine*, entity, uint16_t numquads);
void addcomponent(engine*, entity, const char* name);
void setattr(engine*, entity, const char* attr, int argv, char** argc);

texture addtex(renderer*, const char* filename);

// sprite manipulation functions
void moveto(renderer*, sprite, uint16_t x, uint16_t y);
void moveby(renderer*, sprite, int dx, int dy);
void setsize(renderer*, sprite, uint16_t quad, uint16_t w, uint16_t h);
void setbounds(renderer*, sprite, uint16_t quad, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void setuv(renderer*, sprite, uint16_t quad, float tlx, float tly, float w, float y);
void settex(renderer*, sprite, texture);



stream_id play_track(audio*, uint32_t track_id);
void stop_stream(audio*, stream_id track);
void set_channel_volume(audio*, sink_channel channel, float volume);


void run(engine* e, const char* scriptname);


// non-scriptable functions
texture addtex(renderer*, const uint8_t* buf, unsigned w, unsigned h);
void loadscript(engine* e, const char* name, int argc, const char** argv);


#endif //ENGINE_H
