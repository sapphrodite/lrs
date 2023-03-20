#include "ecs.h"
#include "engine.h"
#include <algorithm>

namespace ecs{ 

	void run_physics(engine* e, pool<display>& dpy, pool<physics>& phys) {
		for (auto& p : phys) {
			p.velocity.x += p.accel.x;
			p.velocity.y += p.accel.y;

		    p.velocity.x = std::clamp(p.velocity.x, -p.velocity_cap.x, p.velocity_cap.x);
		    p.velocity.y = std::clamp(p.velocity.y, -p.velocity_cap.y, p.velocity_cap.y);
			

			for (auto& d : dpy) {
				for (auto& s : d.sprites) {
					moveby(get_renderer(e), s, p.velocity.x, p.velocity.y);
				}
			}	
		}
	}

}
