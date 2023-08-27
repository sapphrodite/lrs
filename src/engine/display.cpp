#include "display.h"
#include <include/marked_array.h>
#include <array>
#include <cassert>
#include <cstring> //memcpy
#include <GL/glew.h>
#include <libpng/png.h>
#include <SDL2/SDL.h>

struct spritedata {
	std::vector<vertex> data;
	u32 tex;
};

// can eventually automatically bind associated normal/height maps for a tex
class texture_manager {
public:
	texture_manager() : textures(64) {};
	u32 add(const u8* buf, u32 w, u32 h) { return textures.insert(texgen(buf, w, h)); }
	void use(size_t index) { glBindTexture(GL_TEXTURE_2D, textures[index]); }
private:
	marked_vector<u32> textures;

	u32 texgen(const u8* buf, u32 width, u32 height) {
		u32 texid = 0;
		glGenTextures(1, &texid);
		glBindTexture(GL_TEXTURE_2D, texid);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		// TODO - actually account for texture data type
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);
		return texid;
	}
};

// Class to facilitate sprite batching
struct batcher {
	batcher() {
		glGenBuffers(1, &vbo_id);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
		lock();
	}

	size_t size() { return batched; }
	size_t capacity() { return 4096; }

	// returns the number of vertices read
	size_t add(vertex* in, size_t num_verts) {
		size_t verts_to_copy = std::min(num_verts, capacity() - batched);
		memcpy(buf + batched, in, verts_to_copy * sizeof(vertex));
		batched += verts_to_copy;
		return verts_to_copy;
	}

	void release() { glUnmapBuffer(GL_ARRAY_BUFFER); }
	void lock()  {
		glBufferData(GL_ARRAY_BUFFER, capacity() * sizeof(vertex), nullptr, GL_STREAM_DRAW);
		buf = (vertex*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		batched = 0;
	}
private:
	vertex* buf = nullptr;
	size_t batched = 0;
	u32 vbo_id = 0;
	u32 vao_id = 0;
};

// TODO - this is a bad decision
class camera_manager {
public:
	void set_attr_idx(u32 attr_idx) { cam_attr_idx = attr_idx; }
	void set_camera(vec2<f32> pos) {
		sprite_mat[12] = -1.0f + (pos.x * sprite_mat[0]);
		sprite_mat[13] = 1.0f +  (pos.y * sprite_mat[5]);
	}

	void set_res(vec2<u16> res) {
		glViewport(0, 0, res.x, res.y);
		sprite_mat[0] = 2.0f / f32(res.x);
		sprite_mat[5] = -2.0f / f32(res.y);

		ui_mat[0] = 2.0f / f32(res.x);
		ui_mat[5] = -2.0f / f32(res.y);

		// UI camera never moves, update translation values here instead
		ui_mat[12] = -1;
		ui_mat[13] = 1;
	}

	void use_sprite_cam() { use_cam(sprite_mat.data()); }
	void use_ui_cam() { use_cam(ui_mat.data()); }
private:
	void use_cam(f32* data) {
		glUniformMatrix4fv(cam_attr_idx, 1, GL_FALSE, data);
	}

	u32 cam_attr_idx = 0;
	vec2<f32> res_scale;
	std::array<f32, 16> sprite_mat = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,};
	std::array<f32, 16> ui_mat;
};

class shader {
public:
	shader() = default;
	shader(const char* vertpath, const char* fragpath) {
		glDeleteProgram(id);
		u32 vertID = load_stage(vertpath, GL_VERTEX_SHADER);
		u32 fragID = load_stage(fragpath, GL_FRAGMENT_SHADER);
		u32 progID = glCreateProgram();
		glAttachShader(progID, vertID);
		glAttachShader(progID, fragID);
		glLinkProgram(progID);
		glDetachShader(progID, vertID);
		glDetachShader(progID, fragID);
		glDeleteShader(vertID);
		glDeleteShader(fragID);
		id = progID;
	}

	void use() { glUseProgram(id); }
	u32 uniform_id(const char* name) { return glGetUniformLocation(id, name); }
	u32 load_stage(const char* fname, GLenum type) {
		char* buf = new char[8192];
		FILE* fp = fopen(fname, "r");
		int bread = fread(buf, 1, 8192, fp);
		if (bread > 8192)
			printf("oops!\n");
		fclose(fp);
		u32 ID = glCreateShader(type);
		glShaderSource(ID, 1, &buf, 0);
		glCompileShader(ID);
		delete[] buf;
		return ID;
	}
private:
	u32 id = 0;
};

// love 2 debug :D
void GLAPIENTRY MessageCallback(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
	fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
				 ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ), type, severity, message );
}

class renderer {
public:
	renderer() {
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(MessageCallback, 0);

		std::array<u16, 4096 * 6> index_buffer{};
		u8 lookup[] = {0, 1, 3, 3, 2, 0};
		for (size_t i = 0; i < 4096; i++)
			index_buffer[i] = ((i / 6) * 4) + lookup[i % 6];
		glGenBuffers(1, &ebo_id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer.size() * sizeof(u16), &index_buffer[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(vertex), 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*) offsetof(vertex, uv));
	}

	void render() {
		camera.use_sprite_cam();
		u16 last_tex = 0;
		for (auto& s : objs) {
			if (s.tex != last_tex)
				draw_batch();
			last_tex = s.tex;

			vertex* pos = s.data.data();
			vertex* end = s.data.data() + s.data.size();
			while (pos < end) {
				pos += batch.add(pos, end - pos);
				if (pos < end)
					draw_batch();
			}
		}
		draw_batch();
	}

	void load_shader(const char* vertpath, const char* fragpath) {
		shader s(vertpath, fragpath);
		s.use();
		shaders.emplace_back(s);
		camera.set_attr_idx(s.uniform_id("cam"));
		camera.use_sprite_cam();
	}

	void draw_batch() {
		batch.release();
		glDrawElements(GL_TRIANGLES, (batch.size() / 4) * 6, GL_UNSIGNED_SHORT, (void*) 0);
		batch.lock();
	}

	std::vector<shader> shaders;
	u32 ebo_id;
	marked_vector<spritedata> objs;

	camera_manager camera;
	batcher batch;
	texture_manager textures;
};


class window {
public:
	window() {
		win= SDL_CreateWindow("test", 0, 0, 0, 0, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
		ctx = SDL_GL_CreateContext(win);
	}

	~window() {
		SDL_DestroyWindow(win);
		SDL_Quit();
		SDL_GL_DeleteContext(ctx);
	}

	void swap_buffers() { SDL_GL_SwapWindow(win); }
	void set_resolution(vec2<u16> res) { SDL_SetWindowSize(win, res.x, res.y); }

	int poll_events(display::event* wev) {
		SDL_Event event;
		int retval = SDL_PollEvent(&event);
		wev->event = display::event::type::null;
		switch (event.type) {
			case SDL_QUIT:
				wev->event = display::event::type::quit;
				break;
			case SDL_KEYUP:
			case SDL_KEYDOWN: {
				if (event.key.repeat) break;
				wev->event = display::event::type::keyevent;
				wev->data[0] = event.key.keysym.sym;
				wev->data[1] = event.type == SDL_KEYUP ? 0 : 1;
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP: {
				wev->event = display::event::type::mousebutton;
				wev->data[0] = event.button.x;
				wev->data[1] = event.button.y;
				wev->data[2] = event.type == SDL_MOUSEBUTTONUP ? 0 : 1;
				break;
			}
			case SDL_MOUSEMOTION: {
				wev->event = display::event::type::cursor;
				wev->data[0] = event.motion.x;
				wev->data[1] = event.motion.y;
				break;
			}
			case SDL_WINDOWEVENT: {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						wev->event = display::event::type::resize;
						wev->data[0] = event.window.data1;
						wev->data[1] = event.window.data2;
						break;
					default:
						wev->event = display::event::type::null;
						break;
				}
				break;
			}
		}
		return retval;
	}
private:
	SDL_Window* win;
	SDL_GLContext ctx;
};


class display::pimpl {
public:
	window* w;
	renderer* r;
};

display::display() {
	data = new pimpl;
	data->w = new window;
	glewInit();
	data->r = new renderer;
}

display::~display() {
	delete data->w;
	delete data->r;
	delete data;
}

void display::set_vsync(bool state) { SDL_GL_SetSwapInterval(state ? 1 : 0); }
void display::render() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	data->r->render();
	data->w->swap_buffers();
}
int display::poll_events(display::event* wev) { return data->w->poll_events(wev); }

void display::load_shader(const char* vpath, const char* fpath) {
	data->r->load_shader(vpath, fpath);
}

void display::set_cam(f32 cam_x, f32 cam_y) {
	data->r->camera.set_camera(vec2<f32>(cam_x, cam_y)); 
}

void display:: set_res(u16 res_x, u16 res_y) {
	data->r->camera.set_res(vec2<u16>(res_x, res_y)); 
 	data->w->set_resolution({res_x, res_y});
}

u16 display::addtex(const char* filename) {
	FILE* fp = fopen(filename, "rb");
	assert(fp);

	png_struct* png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_info* info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);
	png_read_update_info(png_ptr, info_ptr);

	int width = png_get_image_width(png_ptr, info_ptr);
	int height = png_get_image_height(png_ptr, info_ptr);
	int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	u8* buffer = new u8[height * rowbytes];
	std::vector<png_byte*> ptr_storage(height);
	for (int y = 0; y < height; y++)
		ptr_storage[y] = (u8*) buffer + (rowbytes * y);
	png_read_image(png_ptr, ptr_storage.data());
	u32 texid = data->r->textures.add(buffer, width, height);
	delete[] buffer;

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	return texid;
}

u16 display::obj_add(u16 ntiles) {
	u16 obj = data->r->objs.insert(spritedata());
	obj_resize(obj, ntiles);
	return obj;
}

void display::obj_delete(u16 obj) { data->r->objs.remove(obj); }
void display::obj_resize(u16 obj, u16 ntiles) { data->r->objs[obj].data.resize(ntiles * 4); }
void display::obj_settex(u16 obj, u16 tex) { data->r->objs[obj].tex = tex; }
void display::obj_moveto(u16 obj, vec2i pos) {
	for (size_t i = 0; i < data->r->objs[obj].data.size() / 4; i++)
		tile_setpos(obj, i, pos);
}

void display::obj_moveby(u16 obj, vec2i delta) {
	vec2i pos = data->r->objs[obj].data[0].pos.to<int>();
	for (size_t i = 0; i < data->r->objs[obj].data.size() / 4; i++)
		tile_setpos(obj, i, pos + delta);
}

void display::tile_setpos(u16 obj, u16 tile, vec2i pos) {
	spritedata& spr = data->r->objs[obj];
	vec2i size = (spr.data[tile * 4 + 3].pos - spr.data[tile * 4].pos).to<int>();
	tile_setbounds(obj, tile, pos, size);
}

void display::tile_setsize(u16 obj, u16 tile, vec2i size) {
	vec2i pos = data->r->objs[obj].data[tile * 4].pos.to<int>();
	tile_setbounds(obj, tile, pos, size);
}

void display::tile_setbounds(u16 obj, u16 tile, vec2i pos, vec2i size) {
	spritedata& spr = data->r->objs[obj];
	spr.data[tile * 4].pos = pos.to<u16>();
	spr.data[tile * 4 + 1].pos = vec2i{pos.x + size.x, pos.y}.to<u16>();
	spr.data[tile * 4 + 2].pos = vec2i{pos.x, pos.y + size.y}.to<u16>();
	spr.data[tile * 4 + 3].pos = vec2i{pos.x + size.x, pos.y + size.y}.to<u16>();
}

void display::tile_setuv(u16 obj, u16 tile, vec2f uv, vec2f size) {
	spritedata& spr = data->r->objs[obj];
	spr.data[tile * 4].uv = uv;
	spr.data[tile * 4 + 1].uv = vec2f{uv.x + size.x, uv.y};
	spr.data[tile * 4 + 2].uv = vec2f{uv.x, uv.y + size.y};
	spr.data[tile * 4 + 3].uv = vec2f{uv.x + size.x, uv.y + size.y};
}
