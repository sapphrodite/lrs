#include "ecs.h"
#include "display.h"
#include <include/marked_array.h>
#include <algorithm>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

template <typename T> using pool = marked_vector<T>;
template <typename T> struct type_tag {};

struct component {
	u16 entity;
};

struct render : public component {
	std::vector<uint32_t> objects;
};

struct physics : public component {
	struct force {
		vec2f f;
		int lifetime;
	};
	std::vector<force> forces;

	vec2f accum = vec2f(0, 0);
	vec2f velocity;
};

void run_physics(display* dpy, pool<render>& r, pool<physics>& phys) {
	for (auto& p : phys) {
		// air resistance is f = res * -v^2, when object is moving
		vec2f res(1, 1);
		if (p.velocity.x != 0 || p.velocity.y != 0) {
			vec2f f = (p.velocity * p.velocity) * res;
			f.x = p.velocity.x > 0 ? -f.x : f.x;
			f.y = p.velocity.y > 0 ? -f.y : f.y;
			p.forces.emplace_back(physics::force{f, 1});
		}

		// f=ma, and a = d/dt v, so v += f / m for each tick
		for (auto& force : p.forces) {
			p.velocity += force.f / vec2f(100, 100);
			if (--force.lifetime == 0) {
				std::swap(force, p.forces.back());
				p.forces.erase(p.forces.end() - 1);
			}
		}

		p.accum += p.velocity;
		vec2i delta = p.accum.to<int>();
		p.accum -= delta.to<float>();

		for (auto& obj : r[p.entity].objects) {
			dpy->obj_moveby(obj, delta);
		}
	}
}

// an entity manager
class ecs::pimpl {
public:
	#define ALL_COMPONENTS(m) \
		m(render) m(physics)

	// A series of higher-order macros to prevent duplicating function calls for each component type
	#define POOL_NAME(T) T ## _pool
	#define GENERATE_ACCESS_FUNCTIONS(T) constexpr pool<T>& get_pool(type_tag<T>) { return POOL_NAME(T); }
	#define GENERATE_POOLS(T) pool<T> POOL_NAME(T);
	#define GENERATE_REMOVE_CALLS(T) data->POOL_NAME(T).remove(entity);
	#define GENERATE_RESIZE_CALLS(T) POOL_NAME(T).set_capacity(new_size);

	pimpl() { resize(128); }
	template <typename T> T& add(u16 entity) {
		assert(entities.get(entity));
		T& c = get_pool<T>().insert(entity);
		c.entity = entity;
		return c;
	}

	template <typename T> T& get(u16 entity) {
		assert(entities.get(entity));
		return get_pool<T>()[entity];
	}

	template <typename T> void remove(u16 entity) {
		assert(entities.get(entity));
		get_pool<T>().remove(entity);
	}

	template <typename T>
	bool exists(u16 entity) { return get_pool<T>().exists(entity); }
	template <typename T>
	pool<T>& get_pool() { return get_pool(type_tag<T>()); }

	void resize(size_t new_size) {
		entities.resize(new_size);
		ALL_COMPONENTS(GENERATE_RESIZE_CALLS)
	}

	bitvector entities;
	ALL_COMPONENTS(GENERATE_ACCESS_FUNCTIONS)
	ALL_COMPONENTS(GENERATE_POOLS)
};

ecs::ecs(display* eng) : eng(eng) { data = new pimpl; }
ecs::~ecs() { delete data; }

u8 ecs::entity_add() { return data->entities.insert(); }
void ecs::entity_remove(u8 entity) {
	data->entities.clear(entity);
	ALL_COMPONENTS(GENERATE_REMOVE_CALLS)
}

void ecs::component_add(u8 entity, const char* name) {
	#define GENERATE_STRCMP_ADD(T) \
	if (strcmp(name, #T) == 0) { \
		data->add<T>(entity); \
		return; \
	}
	ALL_COMPONENTS(GENERATE_STRCMP_ADD)
}

void ecs::component_remove(u8 entity, const char* name) {
	#define GENERATE_STRCMP_REM(T) \
	if (strcmp(name, #T) == 0) { \
		data->remove<T>(entity); \
		return; \
	}
	ALL_COMPONENTS(GENERATE_STRCMP_REM)
}

void ecs::obj_register(u8 entity, u16 obj) {
	if (!data->exists<render>(entity))
		printf("Error in obj_register: entity must have render component!\n");
	else
		data->get<render>(entity).objects.emplace_back(obj);
}

void ecs::add_force(u8 entity, vec2f f, int lifetime) {
	physics::force force{f, lifetime};
	if (!data->exists<render>(entity))
		printf("Error in add_force: entity must have physics component!\n");
	else
		data->get<physics>(entity).forces.emplace_back(force);
}

void ecs::run_tick() {
	run_physics(eng, data->get_pool<render>(), data->get_pool<physics>());
}
