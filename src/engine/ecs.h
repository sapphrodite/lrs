#ifndef ECS_H
#define ECS_H

#include <common/marked_array.h>

namespace ecs {
    using entity = uint8_t;

    template <typename T>
    struct type_tag {};

    struct component {
        entity entity_id;
    };

    struct display : component {};

    template <typename T>
    using pool = marked_vector<T>;

    #define ALL_COMPONENTS(m) \
        m(display)

    class entity_manager {
        // A series of higher-order macros to prevent duplicating function calls for each component type
        #define POOL_NAME(T) T ## _pool
        #define GENERATE_ACCESS_FUNCTIONS(T) constexpr pool<T>& get_pool(type_tag<T>) { return POOL_NAME(T); }
        #define GENERATE_REMOVE_CALLS(T) POOL_NAME(T).remove(e);
        #define GENERATE_POOLS(T) pool<T> POOL_NAME(T);
    public:
        template <typename T>
        T& add_component(entity e) {
            assertion(entities.get(e), "Cannot modify nonexistent entity");
            T& c = get_pool<T>().emplace(e);
            c.entity_id = e;
            return c;
        }
        template <typename T>
        T& get_component(entity e) {
            assertion(entities.get(e), "Cannot modify nonexistent entity");
            return get_pool<T>()[e];
        }
        template <typename T>
        T& remove_component(entity e) {
            assertion(entities.get(e), "Cannot modify nonexistent entity");
            get_pool<T>().remove(e);
        }

        // put string-based comp. type lookup for cmd processing here :D

        template <typename T>
        pool<T>& get_pool() { return get_pool(type_tag<T>()); }

        entity add_entity() {
            entity e = entities.least_unset_bit();
            // add provision to handle resize
            // also need to resize component pools
            entities.set(e);
            return e;
        }
        entity remove_entity(entity e) {
            entities.clear(e);
            ALL_COMPONENTS(GENERATE_REMOVE_CALLS)
        }
    private:
        bitvector entities;
        ALL_COMPONENTS(GENERATE_ACCESS_FUNCTIONS)
        ALL_COMPONENTS(GENERATE_POOLS)
    };
}

#endif //ECS_H
