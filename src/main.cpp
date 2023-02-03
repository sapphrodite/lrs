#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <libpng/png.h>

#include <vector>
#include <array>
#include <string>
#include <cassert>

#include <common/marked_array.h>
#include <common/coordinate_types.h>

#include <engine/engine.h>

// both this and the SDL window class should prevent copying or moving, copy over the support class eventually
// I bet you there's a bug that occurs on subsequent calls to read_image()  ... :)
class png_reader {
public:
    png_reader(const char* filename) {
        fp = fopen(filename, "rb");
        assert(fp);
        char header[8];    // 8 is the maximum size that can be checked
        fread(header, 1, 8, fp);  // This advances the read pointer - do not remove

        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        info_ptr = png_create_info_struct(png_ptr);

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);
        png_read_info(png_ptr, info_ptr);
        png_read_update_info(png_ptr, info_ptr);
    }
    ~png_reader() {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
    }

    size_t width() { return png_get_image_width(png_ptr, info_ptr); }
    size_t height() { return png_get_image_height(png_ptr, info_ptr); }
    size_t lenbytes() { return height() * rowbytes(); }
    size_t read_image(uint8_t* buf) {
        // What in the actual fuck is this? thanks libpng
        std::vector<png_byte*> ptr_storage(height());
        for (int y = 0; y < height(); y++) {
            ptr_storage[y] = buf + (rowbytes() * y);
        }
        png_read_image(png_ptr, ptr_storage.data());
        return lenbytes();
    }
private:
    size_t rowbytes() { return png_get_rowbytes(png_ptr, info_ptr); }

    FILE* fp = nullptr;
    png_struct* png_ptr = nullptr;
    png_info* info_ptr = nullptr;
};


// class for managing window events
struct window_event {
    enum class evtype {
        null, quit, resize, keyevent, mousebutton, cursor
    } type;
    uint32_t data[8]; // 32 bytes of data buffer
};


// RAII wrapper for telling the OS how to create and manage our OpenGL window
class sdl_window {
public:
    sdl_window() {
        window = SDL_CreateWindow("test", 0, 0, 0, 0, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
        ctx = SDL_GL_CreateContext(window);
    }
    ~sdl_window() {
        SDL_DestroyWindow(window);
        SDL_Quit();
        SDL_GL_DeleteContext(ctx);
    }
    void set_vsync(bool state) { SDL_GL_SetSwapInterval(state ? 1 : 0); }
    void swap_buffers() { SDL_GL_SwapWindow(window); }
    void set_resolution(vec2<uint16_t> res) { SDL_SetWindowSize(window, res.x, res.y); }

    int poll_events(window_event* wev) {
        SDL_Event event;
        int retval = SDL_PollEvent(&event);
        wev->type = window_event::evtype::null;
        switch (event.type) {
            case SDL_QUIT:
                wev->type = window_event::evtype::quit;
                break;
            case SDL_KEYUP:
            case SDL_KEYDOWN: {
                if (event.key.repeat) break;
                wev->type = window_event::evtype::keyevent;
                wev->data[0] = event.key.keysym.sym;
                wev->data[1] = event.type == SDL_KEYUP ? 0 : 1;
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                wev->type = window_event::evtype::mousebutton;
                wev->data[0] = event.button.x;
                wev->data[1] = event.button.y;
                wev->data[2] = event.type == SDL_MOUSEBUTTONUP ? 0 : 1;
                break;
            }
            case SDL_MOUSEMOTION: {
                wev->type = window_event::evtype::cursor;
                wev->data[0] = event.motion.x;
                wev->data[1] = event.motion.y;
                break;
            }
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        wev->type = window_event::evtype::resize;
                        wev->data[0] = event.window.data1;
                        wev->data[1] = event.window.data2;
                        break;
                    default:
                        wev->type = window_event::evtype::null;
                        break;
                }
                break;
            }
        }
        return retval;
    }
private:
    SDL_Window* window;
    SDL_GLContext ctx;
};

// as it stands, this function works, although it doesn't preserve the '='...
size_t parse(const char* cmd, char** data_out) {
    size_t read_idx = 0;
    size_t write_idx = 0;
    size_t word_idx = 0;
    bool skipping = false;
    for (;;) {
        switch (cmd[read_idx]) {
        case ' ':
        case '=':
        case '\t':
            if (skipping) {
                break;
            }
            data_out[word_idx][write_idx] = 0;
            word_idx++;
            write_idx = 0;
            skipping = true;
            break;
        case '\n':
        case '\0':
            return word_idx;
        default:
            data_out[word_idx][write_idx++] = cmd[read_idx];
            skipping = false;
            break;
        }
        read_idx++;
    }
}

#include <engine/parser.h>

int main() {
    auto maps_dict = dsl::parser::parse_file("fuck.txt");
    for (auto& [key, val] : maps_dict->data()) {
	const auto* l = reinterpret_cast<const dsl::function*>(val.get());
        printf("test!");
    }

    // welcome to paradise....
    char buf [256 * 256];
    char* data[256];
    for (int i = 0; i < 256; i++) {
        data[i] = &buf[i * 256];
    }
    parse("$1=shit bar blah", data);


    sdl_window window;
    window.set_vsync(true);

    // GLEW is a little library that provides a nice interface to the OpenGL driver
    // the alternative is eldritch magic even I fear to posess...
    glewInit();


    engine e;


    // interleave in (x, y, u, v) pattern, counting clockwise
    // each triangle will have 3 vertices, which means 2 shared vertices.
    // indexing can improve this later, but let's not worry for right now...
    sprite s;
    s.data = std::vector<vertex>{
        vertex{{90, 10}, {1.0, 0.0}},
        vertex{{10, 10}, {0.0, 0.0}},
        vertex{{10, 90}, {0.0, 1.0}},
        vertex{{90, 90}, {1.0, 1.0}},
    };

    e.r.add_sprite(s);

    window.set_resolution({1024, 768});
    e.r.set_resolution({1024, 768});
    e.r.set_camera({0, 0});
    // load img data here
    uint8_t buffer[256 * 256 * 4];
    png_reader reader("test.png");
    size_t image_len = reader.read_image(buffer);
    s.tex_id = e.r.add_texture(buffer, reader.width(), reader.height());

/*    audio::engine audio;
    // Audio playback test
    audio::stream_id fuck = audio.play_track(0);
    audio.mix_audio(48000 * 2);*/

    while (1) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        e.r.render();
        window.swap_buffers();

        window_event wev;
        while (window.poll_events(&wev)) {
            switch (wev.type) {
                case window_event::evtype::quit:
                    return 0;
                case window_event::evtype::resize:
                    e.r.set_resolution({wev.data[0], wev.data[1]});
                default:
                    break;
            }
        }
    }
}
