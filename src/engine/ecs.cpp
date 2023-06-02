#include "ecs.h"
#include "engine.h"
#include <algorithm>

namespace ecs {
	template <typename T> int sgn(T val) {
	    return (T(0) < val) - (val < T(0));
	}

	void run_physics(engine* e, pool<display>& dpy, pool<physics>& phys) {
		for (auto& p : phys) {
/*			float current_angle = atan2(p.accel.x, p.accel.y);
			float r = -0.25; // i love hardcoding
			float accel_angle = current_angle + sgn(r) * (M_PI / 2);

			if (accel_angle != 0) {
				vec2<float> rot_accel = vec2<float>(abs(r) * cos(accel_angle) + abs(r) * sin(accel_angle));
				p.accel += rot_accel;

				float new_angle = atan2(p.accel.x, p.accel.y);
				for (auto& s : dpy[p.entity_id].sprites) {
					rotate(e, p.entity_id, s, new_angle - current_angle);
				}
			}

			p.velocity.x += p.accel.x;
			p.velocity.y += p.accel.y;


		    p.velocity.x = std::clamp(p.velocity.x, -p.velocity_cap.x, p.velocity_cap.x);
		    p.velocity.y = std::clamp(p.velocity.y, -p.velocity_cap.y, p.velocity_cap.y);*/
			int frac = 1 << 6;
			int air_res = 1 << 13;

			for (auto& force : p.forces) {
				p.velocity += force.f / vec2<int>(p.mass, p.mass);

				if (--force.lifetime == 0) {
					std::swap(force, p.forces.back());
					p.forces.erase(p.forces.begin() + p.forces.size() - 1);
					continue;
				}
			}

			if (p.velocity.x != 0 && p.velocity.y != 0) {
				vec2<int> v2 = p.velocity * p.velocity;
				v2 += vec2<int>(air_res, air_res);
				v2.x = p.velocity.x > 0 ? -v2.x : v2.x;
				v2.y = p.velocity.y > 0 ? -v2.y : v2.y;
				addforce(e, p.entity_id, v2.x / air_res, v2.y / air_res, 1, false);
			}

			p.accum += p.velocity;
			vec2<int> pix_delta(p.accum.x / frac, p.accum.y / frac);
			p.accum -= vec2<int>(pix_delta.x * frac, pix_delta.y * frac);

			if (pix_delta.x != 0 && pix_delta.y != 0) {
				for (auto& s : dpy[p.entity_id].sprites) {
					moveby(e, p.entity_id, s, pix_delta.x, pix_delta.y);
				}
			}
		}
	}

	void get_normals(vec2<int>* normals, collision::hitbox& hb) {
		for (size_t i = 0; i < 4; i++) {
			vec2<int> edge = hb[(i + 1) % 4].to<int>() - hb[i].to<int>();
			normals[i] = vec2<int>(-edge.y, edge.x);
		}
	}

	// Project the vertices of the shape onto each normal, to determine overlaps and gaps
	void project_shape(vec2<int> normal, collision::hitbox& hb, float& min, float& max) {
	    for (int i = 0; i < 4; i++) {
	        float projection = (hb[i].x * normal.x) + (hb[i].y * normal.y);
	        min = std::min(min, projection);
    	    max = std::max(max, projection);
	    }
	}

	bool hitbox_overlap(collision::hitbox& hb1, collision::hitbox& hb2) {
		vec2<int> normals[8];
		get_normals(normals, hb1);
		get_normals(normals + 4, hb2);

		for (int i = 0; i < 8; i++) {
        	float a_min = 65535, a_max = 0, b_min = 65535, b_max = 0;

	        project_shape(normals[i], hb1, a_min, a_max);
	        project_shape(normals[i], hb2, b_min, b_max);

	        bool a = (a_min < b_max) && (a_min > b_min);
	        bool b = (b_min < a_max) && (b_min > a_min);
	        if ((a || b) == false) {
	            return false;
	        }
		}
		return true;
	}


	void run_collision(pool<collision>& col) {
		
		for (auto it1 = col.begin(); it1 != col.end(); ++it1) {

			auto it2 = it1;
			++it2;
			for (; it2 != col.end(); ++it2) {

				for (auto& hb1 : (*it1).hitboxes) {
					for (auto& hb2 : (*it2).hitboxes) {
						if (hitbox_overlap(hb1, hb2)) {
							printf("Objects collided!\n");
						}
					}
				}
			}
		}
 	}

	// more higher-order macros
    #define GENERATE_REMOVE_CALLS(T) POOL_NAME(T).remove(e);
    #define GENERATE_RESIZE_CALLS(T) POOL_NAME(T).set_capacity(new_size);
	#define GEN_ADD_INST(T) template T& entity_manager::add<T>(entity e);
	#define GEN_GET_INST(T) template T& entity_manager::get<T>(entity e);
	#define GEN_REMOVE_INST(T) template void entity_manager::remove<T>(entity e);
	#define GEN_EXISTS_INST(T) template bool entity_manager::exists<T>(entity e);
	#define GEN_GETPOOL_INST(T) template pool<T>& entity_manager::get_pool<T>();

	ALL_COMPONENTS(GEN_ADD_INST)
	ALL_COMPONENTS(GEN_GET_INST)
	ALL_COMPONENTS(GEN_REMOVE_INST)
	ALL_COMPONENTS(GEN_EXISTS_INST)
	ALL_COMPONENTS(GEN_GETPOOL_INST)

    template <typename T> T& entity_manager::add(entity e) {
        assertion(entities.get(e), "Cannot modify nonexistent entity");
        T& c = get_pool<T>().insert(e, T());
        c.entity_id = e;
        return c;
    }
    template <typename T> T& entity_manager::get(entity e) {
        assertion(entities.get(e), "Cannot modify nonexistent entity");
        return get_pool<T>()[e];
    }
    template <typename T> void entity_manager::remove(entity e) {
        assertion(entities.get(e), "Cannot modify nonexistent entity");
        get_pool<T>().remove(e);
    }
    template <typename T> bool entity_manager::exists(entity e) {
        return get_pool<T>().exists(e);
    }
    template <typename T>
    pool<T>& entity_manager::get_pool() { return get_pool(type_tag<T>()); }

    entity entity_manager::add_entity() {
        entity e = entities.least_unset_bit();
        // add provision to handle resize
        // also need to resize component pools
        entities.set(e);
        return e;
    }
    void entity_manager::remove_entity(entity e) {
        entities.clear(e);
        ALL_COMPONENTS(GENERATE_REMOVE_CALLS)
    }
	void entity_manager::resize(size_t new_size) {
        entities.resize(new_size);
        ALL_COMPONENTS(GENERATE_RESIZE_CALLS)
	}



}
