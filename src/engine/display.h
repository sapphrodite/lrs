#ifndef MODULES_H
#define MODULES_H

#include <include/integral_types.h>
#include <include/coordinate_types.h>
#include <stdint.h>

class display {
public:
	display();
	~display();

	struct event {
		enum type {
		    null, quit, resize, keyevent, mousebutton, cursor
		} event;
		uint32_t data[8]; // Contains event data such as click position
	};

	i32 poll_events(event* wev);

	void set_cam(f32 cam_x, f32 cam_y);
	void set_res(u16 res_x, u16 res_y);
	void set_vsync(bool state);
	void load_shader(const char* vertpath, const char* fragpath);
	void render();

	u16 addtex(const char* filename);

	u16 obj_add(u16 ntiles);
	void obj_delete(u16 obj);
	void obj_resize(u16 obj, u16 ntiles);
	void obj_settex(u16 obj, u16 tex);
	void obj_moveto(u16 obj, vec2i pos);
	void obj_moveby(u16 obj, vec2i delta);
	void tile_setpos(u16 obj, u16 tile, vec2i pos);
	void tile_setsize(u16 obj, u16 tile, vec2i size);
	void tile_setbounds(u16 obj, u16 tile, vec2i pos, vec2i size);
	void tile_setuv(u16 obj, u16 tile, vec2f uv, vec2f size);
private:
	class pimpl;
	pimpl* data;
};

#endif //MODULES_H
