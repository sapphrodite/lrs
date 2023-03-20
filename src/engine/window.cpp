#include <SDL2/SDL.h>
#include <common/coordinate_types.h>
#include "modules.h"


// RAII wrapper for telling the OS how to create and manage our OpenGL window
class window {
public:
    window() {
        win= SDL_CreateWindow("test", 0, 0, 0, 0, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
        ctx = SDL_GL_CreateContext(win);
    }
    ~window() {
        SDL_DestroyWindow(win);
        SDL_Quit();
        SDL_GL_DeleteContext(ctx);
    }
    void swap_buffers() { SDL_GL_SwapWindow(win); }
    void set_resolution(vec2<uint16_t> res) { SDL_SetWindowSize(win, res.x, res.y); }

    int poll_events(window_event* wev) {
        SDL_Event event;
        int retval = SDL_PollEvent(&event);
        wev->event = window_event::type::null;
        switch (event.type) {
            case SDL_QUIT:
                wev->event = window_event::type::quit;
                break;
            case SDL_KEYUP:
            case SDL_KEYDOWN: {
                if (event.key.repeat) break;
                wev->event = window_event::type::keyevent;
                wev->data[0] = event.key.keysym.sym;
                wev->data[1] = event.type == SDL_KEYUP ? 0 : 1;
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                wev->event = window_event::type::mousebutton;
                wev->data[0] = event.button.x;
                wev->data[1] = event.button.y;
                wev->data[2] = event.type == SDL_MOUSEBUTTONUP ? 0 : 1;
                break;
            }
            case SDL_MOUSEMOTION: {
                wev->event = window_event::type::cursor;
                wev->data[0] = event.motion.x;
                wev->data[1] = event.motion.y;
                break;
            }
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        wev->event = window_event::type::resize;
                        wev->data[0] = event.window.data1;
                        wev->data[1] = event.window.data2;
                        break;
                    default:
                        wev->event = window_event::type::null;
                        break;
                }
                break;
            }
        }
        return retval;
    }
private:
    SDL_Window* win;
    SDL_GLContext ctx;
};

window* window_alloc() { return new window; }
void window_free(window* w) { delete w; }

void set_vsync(bool state) { SDL_GL_SetSwapInterval(state ? 1 : 0); }
void swap_buffers(window* w) { w->swap_buffers(); }
void set_resolution(window* w, uint16_t res_w, uint16_t res_h) { w->set_resolution({res_w, res_h}); }
int poll_events(window* w, window_event* wev) { return w->poll_events(wev); }
// as it stands, this function works, although it doesn't preserve the '='...
