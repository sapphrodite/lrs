#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

struct engine;

engine* alloc_engine();
void free_engine(engine* e);

bool poll_events(engine*);
void run_tick(engine*);
void render(engine* e);


uint32_t load_tex(engine*, const char* filename);


uint32_t addentity(engine*);
uint32_t addsprite(engine*, uint16_t entity, uint16_t numquads);
void addcomponent(engine*, uint32_t entity, const char* name);
void setattr(engine*, uint32_t entity, const char* attr, int argv, char** argc);


// sprite
void moveto(engine*, uint32_t sprite, uint16_t x, uint16_t y);
void moveby(engine*, uint32_t sprite, int dx, int dy);
void setsize(engine*, uint32_t sprite, uint16_t quad, uint16_t w, uint16_t h);
void setbounds(engine*, uint32_t sprite, uint16_t quad, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void setuv(engine*, uint32_t sprite, uint16_t quad, float tlx, float tly, float w, float y);
void settex(engine*, uint32_t sprite, uint32_t texid);



#endif //ENGINE_H
