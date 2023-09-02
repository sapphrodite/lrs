#ifndef ECS_H
#define ECS_H

#include <include/integral_types.h>
#include <include/coordinate_types.h>

class display;
class ecs {
public:
	ecs(display*);
	~ecs();

	u8 entity_add();
	void entity_remove(u8 entity);
	void component_add(u8 entity, const char* name);
	void component_remove(u8 entity, const char* name);

	// render functions
	void obj_register(u8 entity, u16 obj);

	// physics functions
	void add_force(u8 entity, vec2f f, int lifetime);

	void run_tick();
private:
	struct pimpl;
	pimpl* data;
	display* eng;
};

#endif //ECS_H
