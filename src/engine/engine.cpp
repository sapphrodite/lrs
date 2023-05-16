#include "engine.h"
#include "ecs.h"
#include "modules.h"
#include <interpreter.h>
#include <common/coordinate_types.h>

#include <cstring>
#include <cassert>

#include <map>
#include <string>

class engine {
public:
	engine() {
		w = window_alloc();
		set_resolution(w, 1024, 768);
		set_vsync(true);

		initglew();
		r = renderer_alloc();
		a = audio_alloc();
/*
		audio::initialize(a, 48000, audio::output_mode::stereo);
		audio::load_from_file(a, "440.opus", audio::sink_channel::effects);
		audio::set_channel_volume(a, audio::sink_channel::master, 100);
		audio::set_channel_volume(a, audio::sink_channel::effects, 100);

		int fuck = audio::play_track(a, 0);
		audio::mix_audio(a, 48000 * 2);
*/
		set_res(r, 1024, 768);
		set_cam(r, 0, 0);
	}

	~engine() {
		renderer_free(r);
		audio_free(a);
		window_free(w);
	}
	void render() {
		draw(r);
		swap_buffers(w);
	}

	void run_tick() {
		ecs::run_physics(this, components.get_pool<ecs::display>(), components.get_pool<ecs::physics>());
		ecs::run_collision(components.get_pool<ecs::collision>());
	};

	// these should be shoved in private, once i properly interface them
	renderer* r;
	ecs::entity_manager components;
	window* w;
	audio* a;


	std::map<std::string, std::vector<std::string>> functions;	
};




engine* alloc_engine() { return new engine; }
void free_engine(engine* e) { delete e; }
bool poll_events(engine* e) {
	window_event wev;
	while (poll_events(e->w, &wev)) {
		switch (wev.event) {
			case window_event::type::quit:
				return 0;
			case window_event::type::resize:
				set_res(e->r, wev.data[0], wev.data[1]);
				break;
			default:
				break;
		}
	}
	return 1;
}

void run_tick(engine* e) { e->run_tick(); }
void render(engine* e) { e->render(); }


sprite addentity(engine* eng) {
	uint32_t e = eng->components.add_entity();
	eng->components.add<ecs::display>(e);
	return e;
}

sprite addsprite(engine* eng, entity e, uint16_t numquads) {
	size_t sprid = addsprite(eng->r, numquads);
	eng->components.get<ecs::display>(e).sprites.emplace_back(sprid);
	return sprid;
}



void deserialize(vec2<int>& out, int argc, char** argv) {
	assert(argc == 2);
	out = vec2<int>{std::stoi(argv[0]), std::stoi(argv[1])};
}

void deserialize(vec2<float>& out, int argc, char** argv) {
	assert(argc == 2);
	out = vec2<float>{std::stof(argv[0]), std::stof(argv[1])};
}

void addcomponent(engine* eng, entity e, const char* name) {
#define GENERATE_STRCMP_CALLS(T) \
	else if (strcmp(#T, name) == 0) { \
		eng->components.add<ecs::T>(e); \
	}

	if (!eng) {}
	ALL_COMPONENTS(GENERATE_STRCMP_CALLS)
}

void setattr(engine* eng, entity e, const char* attr, int argc, char** argv) {
	if (strcmp("physics.accel", attr) == 0) {
		deserialize(eng->components.get<ecs::physics>(e).accel, argc, argv);
	} else if (strcmp("physics.velocity_cap", attr) == 0) {
		deserialize(eng->components.get<ecs::physics>(e).velocity_cap, argc, argv);
	}
}

void delta_hb(std::array<vec2<uint16_t>, 4>& hb, int* mat) {
	mat3<int> tfmat;
	memcpy(tfmat.data, mat, 9 * sizeof(int));
	for (auto& vert : hb) {
		vec2<int> res = tfmat * vert.to<int>();
		vert = res.to<uint16_t>();
	}
}


void applydelta(engine* eng, entity e, sprite s, int* mat) {
	if (eng->components.exists<ecs::collision>(e)) {
		auto& col = eng->components.get<ecs::collision>(e);
		for (auto& hb : col.hitboxes)
			delta_hb(hb, mat);
	}

	apply_tf(eng->r, s, mat);
}

// sprite manipulation functions
void moveto(engine* eng, entity e, sprite s, int x, int y) {
	uint16_t ox = 0, oy = 0;
	get_origin(eng->r, s, &ox, &oy);
	moveby(eng, e, s, x - ox, y - oy);
}

void moveby(engine* eng, entity e, sprite s, int dx, int dy) {
	mat3<int> tfmat = mat3<int>::identity();
	tfmat.data[2] = dx;
	tfmat.data[5] = dy;
	applydelta(eng, e, s, tfmat.data);
}
void setbounds(engine* e, sprite s, uint16_t quad, int x, int y, int w, int h) {
	setbounds(e->r, s, quad, x, y, w, h);
}
void setuv(engine* e, sprite s, uint16_t quad, float tlx, float tly, float w, float h) {
	setuv(e->r, s, quad, tlx, tly, w, h);
}
void settex(engine* e, sprite s, texture t) { settex(e->r, s, t); }


void addhitbox(engine* eng, entity e, int x, int y, int w, int h) {
	if (!eng->components.exists<ecs::collision>(e))
		eng->components.add<ecs::collision>(e);

	auto& col = eng->components.get<ecs::collision>(e);
	ecs::collision::hitbox hb;
	hb[0] = vec2<uint16_t>{x, y};
	hb[1] = vec2<uint16_t>{x + w, y};
	hb[2] = vec2<uint16_t>{x, y + h};
	hb[3] = vec2<uint16_t>{x + w, y + h};
	col.hitboxes.emplace_back(hb);
}


texture addtex(engine* e, const uint8_t* buf, unsigned w, unsigned h) {
	 return addtex(e->r, buf, w, h);
}

texture addtex(engine* e, const char* filename) {
	return addtex(e->r, filename);
}


void loadscript(engine* eng, const char* name, int argc, const char** argv) {
	std::vector<std::string> function;
	for (int i = 0; i < argc; i++) {
		function.emplace_back(std::string(argv[i]));
	}

	eng->functions.insert({name, function});
}


void run(engine* e, const char* name) {
	interpreter i;
	i.e = e;


	auto& func = e->functions[name];
	for (auto str : func) {
		i.exec(str.c_str());
	}
}
