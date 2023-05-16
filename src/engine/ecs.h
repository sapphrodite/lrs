#ifndef ECS_H
#define ECS_H

#include <common/marked_array.h>
#include <common/coordinate_types.h>

class engine;
namespace ecs {
    using entity = uint8_t;
    template <typename T> struct type_tag {};
    template <typename T> using pool = marked_vector<T>;

    struct component {
        entity entity_id;
    };

    struct display : public component {
		std::vector<uint32_t> sprites;
	};

    struct physics : public component {
        vec2<float> velocity_cap;
        vec2<float> accel;
        vec2<float> velocity;
    };

	struct collision : public component {
		using hitbox = std::array<vec2<uint16_t>, 4>;
		std::vector<hitbox> hitboxes;
	};

	void run_physics(engine* e, pool<display>& dpy, pool<physics>& phys);
	void run_collision(pool<collision>& col);

    #define ALL_COMPONENTS(m) \
        m(display) m(physics) m(collision)

    // A series of higher-order macros to prevent duplicating function calls for each component type
    #define POOL_NAME(T) T ## _pool
    #define GENERATE_ACCESS_FUNCTIONS(T) constexpr pool<T>& get_pool(type_tag<T>) { return POOL_NAME(T); }
    #define GENERATE_POOLS(T) pool<T> POOL_NAME(T);


    class entity_manager {
    public:
	entity_manager() { resize(128); }
        template <typename T> T& add(entity e);
        template <typename T> T& get(entity e);
        template <typename T> void remove(entity e);
        template <typename T> bool exists(entity e);
        template <typename T> pool<T>& get_pool();

        entity add_entity();
        void remove_entity(entity e);
    private:
		void resize(size_t new_size);

        bitvector entities;
        ALL_COMPONENTS(GENERATE_ACCESS_FUNCTIONS)
        ALL_COMPONENTS(GENERATE_POOLS)
    };
}

#endif //ECS_H
