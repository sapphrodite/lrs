#include "ecs.h"
#include "display.h"
#include <include/marked_array.h>
#include <algorithm>
#include <assert.h>
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

// an entity manager
class ecs::pimpl {
public:
	#define ALL_COMPONENTS(m) \
		m(render)

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
