#include <engine/display.h>
#include <include/stopwatch.h>

int main() {
	display dpy;
	dpy.set_res(1024, 768);
	dpy.load_shader("vert.gls", "frag.gls");

	stopwatch t;
	stopwatch fpscounter;
	size_t lag = 0;
	int numframes = 0;
	t.start();

	u16 tex = dpy.addtex("test.png");
	u16 obj = dpy.obj_add(1);
	dpy.obj_settex(obj, tex);
	dpy.tile_setbounds(obj, 0, {10, 10}, {48, 48});
	dpy.tile_setuv(obj, 0, {0.0, 0.0}, {1.0, 1.0});

	while (1) {
		int us_per_frame = 1000000 / 60;
		lag += t.elapsed<stopwatch::microseconds>();
		t.start();

		display::event wev;
		while (dpy.poll_events(&wev))
			if (wev.event == display::event::quit)
				return 0;

		if (fpscounter.elapsed<stopwatch::seconds>() >= 1.0 ) {
			printf("%f ms/frame\n", 1000.0f / double(numframes));
			numframes = 0;
			fpscounter.start();
		}

		while (lag >= us_per_frame) {
			lag -= us_per_frame;
		}

		numframes++;
		dpy.render();
	}
}
