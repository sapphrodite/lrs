
#include <vector>
#include <array>
#include <string>

#include <engine/engine.h>
#include <interpreter.h>
#include <cassert>

#include <cstring>



#include <common/stopwatch.h>
#include <parser.h>

int main() {

	engine* e = alloc_engine();
	texture tex_id = addtex(get_renderer(e), "test.png");

	interpreter interp;
	interp.e = e;

	const char* buf[256];
	auto maps_dict = dsl::parser::parse_file("fuck.txt");
	for (auto& [key, val] : maps_dict->data()) {
		const auto* l = reinterpret_cast<const dsl::function*>(val.get());
		for (size_t i = 0; i < l->num_commands(); i++) {
			buf[i] = l->read_command(i);
		}
		loadscript(e, key.c_str(), l->num_commands(), buf);
	}

	char buffer[1024];
	for (;;) {
		char* ret = fgets(buffer, 1024, stdin);
		if (ret) {
			interp.exec(ret);
		}
	}

	free_engine(e);
}



int main2() {
	engine* e = alloc_engine();
	texture tex_id = addtex(get_renderer(e), "test.png");

	interpreter interp;
	interp.e = e;



	run(e, "bruh");	
	

	stopwatch t;
	stopwatch fpscounter;
	size_t lag = 0;
	int numframes = 0;

	t.start();

	while (1) {
		int ms_per_frame = 1000000 / 60; 
		lag += t.elapsed<stopwatch::microseconds>();
		t.start();

		if(!poll_events(e))
			break;

		if (fpscounter.elapsed<stopwatch::seconds>() >= 1.0 ) {
			printf("%f ms/frame\n", 1000.0f / double(numframes));
			numframes = 0;
			fpscounter.start();
		}

		while (lag >= stopwatch::microseconds(ms_per_frame).count()) {
			run_tick(e);
			lag -= stopwatch::microseconds(ms_per_frame).count();
		}

		numframes++;
		render(e);
	}

	free_engine(e);
}
