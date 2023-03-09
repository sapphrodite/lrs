#include "engine.h"
#include "ecs.h"
#include "modules.h"

#include <cstring>

class engine {
public:
	engine() {
		w = window::alloc();
		window::set_resolution(w, 1024, 768);
		window::set_vsync(false);

		renderer::glew_init();
		r = renderer::alloc();
		a = audio::alloc();
/*
		audio::initialize(a, 48000, audio::output_mode::stereo);
		audio::load_from_file(a, "440.opus", audio::sink_channel::effects);
		audio::set_channel_volume(a, audio::sink_channel::master, 100);
		audio::set_channel_volume(a, audio::sink_channel::effects, 100);

		int fuck = audio::play_track(a, 0);
		audio::mix_audio(a, 48000 * 2);
*/
		renderer::set_res(r, 1024, 768);
		renderer::set_cam(r, 0, 0);
	}

	~engine() {
		renderer::free(r);
		audio::free(a);
		window::free(w);
	}
	void render() {
		for (auto& s : sprites) {
			renderer::add_sprite(r, s);
		}

	renderer::draw(r);
	window::swap_buffers(w);
		renderer::clear_sprites(r);
	}
	void send_command(int argc, char** argv);
	bool poll_events() {
		window::event wev;
		while (window::poll_events(w, &wev)) {
			switch (wev.type) {
				case window::evtype::quit:
					return 0;
				case window::evtype::resize:
					renderer::set_res(r, wev.data[0], wev.data[1]);
					break;
				default:
					break;
			}
		}
		return 1;
	}

	void run_tick() {
		ecs::run_physics(this, components.get_pool<ecs::display>(), components.get_pool<ecs::physics>());
	};

	// these should be shoved in private, once i properly interface them
	renderer::handle* r;
	ecs::entity_manager components;
	marked_vector<sprite> sprites;
private:
	window::handle* w;
	audio::handle* a;
};


#include <libpng/png.h>
#include <cassert>
// both this and the SDL window class should prevent copying or moving, copy over the support class eventually
// I bet you there's a bug that occurs on subsequent calls to read_image  :)
class png_reader {
public:
	png_reader(const char* filename) {
		fp = fopen(filename, "rb");
		assert(fp);
		char header[8]; // 8 is the maximum size that can be checked
		fread(header, 1, 8, fp);  // This advances the read pointer - do not remove

		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		info_ptr = png_create_info_struct(png_ptr);

		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);
		png_read_info(png_ptr, info_ptr);
		png_read_update_info(png_ptr, info_ptr);
	}
	~png_reader() {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
	}

	size_t width() { return png_get_image_width(png_ptr, info_ptr); }
	size_t height() { return png_get_image_height(png_ptr, info_ptr); }
	size_t lenbytes() { return height() * rowbytes(); }
	size_t read_image(uint8_t* buf) {
		// What in the actual fuck is this? thanks libpng
		std::vector<png_byte*> ptr_storage(height());
		for (int y = 0; y < height(); y++) {
			ptr_storage[y] = buf + (rowbytes() * y);
		}
		png_read_image(png_ptr, ptr_storage.data());
		return lenbytes();
	}
private:
	size_t rowbytes() { return png_get_rowbytes(png_ptr, info_ptr); }

	FILE* fp = nullptr;
	png_struct* png_ptr = nullptr;
	png_info* info_ptr = nullptr;
};

engine* alloc_engine() { return new engine; }
void free_engine(engine* e) { delete e; }

bool poll_events(engine* e) { return e->poll_events(); }
void run_tick(engine* e) { e->run_tick(); }
void render(engine* e) { e->render(); }
uint32_t load_tex(engine* e, const char* filename) {
	// load img data here
	uint8_t buffer[256 * 256 * 4];
	png_reader reader(filename);
	size_t image_len = reader.read_image(buffer);
	return renderer::add_tex(e->r, buffer, reader.width(), reader.height());
}

uint32_t addentity(engine* eng) {
	uint32_t e = eng->components.add_entity();
	eng->components.add<ecs::display>(e);
	return e;
}

uint32_t addsprite(engine* eng, uint16_t e, uint16_t numquads) { 
	auto& dpy = eng->components.get<ecs::display>(e);
	sprite s;
	s.data.resize(numquads * 4);
	size_t sprid = eng->sprites.insert_any(s);

	dpy.sprites.emplace_back(sprid);
	return sprid;
}



vec2<uint16_t> getsize(engine* eng, uint32_t spriteidx, uint16_t quad) {
	sprite& s = eng->sprites[spriteidx];
	uint16_t w = s.data[quad * 4 + 3].pos.x - s.data[quad * 4].pos.x;
	uint16_t h = s.data[quad * 4 + 3].pos.y - s.data[quad * 4].pos.y;
	return vec2<uint16_t>(w, h);
}

void moveby(engine* eng, uint32_t spriteidx, int32_t dx, int32_t dy) {
	sprite& s = eng->sprites[spriteidx];
	for (int quad = 0; quad < s.data.size() / 4; quad++) {
		vec2<uint16_t> origin = eng->sprites[spriteidx].data[quad * 4].pos;
		vec2<uint16_t> size = getsize(eng, spriteidx, quad);
		setbounds(eng, spriteidx, quad, origin.x + dx, origin.y + dy, size.x, size.y);
	}
}

void moveto(engine* eng, uint32_t spriteidx, uint16_t x, uint16_t y) {
	sprite& s = eng->sprites[spriteidx];
	vec2<uint16_t> origin = eng->sprites[spriteidx].data[0].pos;
	moveby(eng, spriteidx, x - origin.x, y - origin.y);
}

void setsize(engine* eng, uint32_t spriteidx, uint16_t quad, uint16_t w, uint16_t h) {
	vec2<uint16_t> origin = eng->sprites[spriteidx].data[quad * 4].pos;
	setbounds(eng, spriteidx, quad, origin.x, origin.y, w, h);
}

void setbounds(engine* eng, uint32_t spriteidx, uint16_t quad, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	sprite& s = eng->sprites[spriteidx];
	s.data[quad * 4].pos = vec2<uint16_t>{x, y};
	s.data[quad * 4 + 1].pos = vec2<uint16_t>{x + w, y}; 
	s.data[quad * 4 + 2].pos = vec2<uint16_t>{x, y + h};
	s.data[quad * 4 + 3].pos = vec2<uint16_t>{x + w, y + h};
}

void setuv(engine* eng, uint32_t spriteidx, uint16_t quad, float x, float y, float w, float h) {
	sprite& s = eng->sprites[spriteidx];
	s.data[quad * 4].uv = vec2<float>{x, y};
	s.data[quad * 4 + 1].uv = vec2<float>{x + w, y}; 
	s.data[quad * 4 + 2].uv = vec2<float>{x, y + h};
	s.data[quad * 4 + 3].uv = vec2<float>{x + w, y + h}; 
}

void settex(engine* eng, uint32_t sprite, uint32_t texid) {
	eng->sprites[sprite].tex_id = texid;
}






void deserialize(vec2<int>& out, int argc, char** argv) {
	assert(argc == 2);
	out = vec2<int>{std::stoi(argv[0]), std::stoi(argv[1])};
}

#define GENERATE_STRCMP_CALLS(T) \
	else if (strcmp(#T, name) == 0) { \
		eng->components.add<ecs::T>(entity); \
	}

void addcomponent(engine* eng, uint32_t entity, const char* name) {
	if (!eng) {}
	ALL_COMPONENTS(GENERATE_STRCMP_CALLS)
}



#define GENERATE_STRCMP_ATTRS(T) \
	else if (strcmp(#T, name) == 0) { \
		eng->components.add<ecs::T>(entity); \
	}


void setattr(engine* eng, uint32_t entity, const char* attr, int argc, char** argv) {

	if (strcmp("physics.accel", attr) == 0) {
		deserialize(eng->components.get<ecs::physics>(entity).accel, argc, argv);
	} else if (strcmp("physics.velocity_cap", attr) == 0) {
		deserialize(eng->components.get<ecs::physics>(entity).velocity_cap, argc, argv);
	}

}
