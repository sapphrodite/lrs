#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

class engine;

using entity = uint16_t;
using sprite = uint16_t;
using texture = uint16_t;

engine* alloc_engine();
void free_engine(engine*);
bool poll_events(engine*);

void run_tick(engine*);
void render(engine*);

entity addentity(engine*);
sprite addsprite(engine*, entity, uint16_t numquads);
void addcomponent(engine*, entity, const char* name);
void setattr(engine*, entity, const char* attr, int argv, char** argc);


// sprite manipulation functions
void moveto(engine*, entity, sprite, int x, int y);
void moveby(engine*, entity, sprite, int dx, int dy);
void setsize(engine*, sprite, uint16_t quad, int w, int h);
void setbounds(engine*, sprite, uint16_t quad, int x, int y, int w, int h);
void setuv(engine*, sprite, uint16_t quad, float tlx, float tly, float w, float y);
void settex(engine*, sprite, texture);
void rotate(engine*, entity, sprite, int deg);


void addhitbox(engine*, entity, int x1, int y1, int x2, int y2);
void addforce(engine*, entity, float x, float y, int lifetime, bool relative);


void run(engine*, const char* scriptname);

// non-scriptable functions
texture addtex(engine*, const uint8_t* buf, unsigned w, unsigned h);
texture addtex(engine*, const char* filename);
void loadscript(engine* e, const char* name, int argc, const char** argv);

#endif //ENGINE_H
