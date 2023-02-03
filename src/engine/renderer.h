#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <array>
#include <stdint.h>

#include <common/coordinate_types.h>
#include <common/marked_array.h>


struct sprite {
    std::vector<vertex> data;
    uint32_t tex_id;
};

class renderer {
public:
    renderer();
    void render();

    uint32_t add_texture(uint8_t* buf, unsigned w, unsigned h);

    void clear_sprites();
    void add_sprite(const sprite& s);

    // sets camera in world coordinates
    void set_camera(vec2<float> pos) { camera.set_camera(pos); }
    void set_resolution(vec2<uint16_t> res) { camera.set_resolution(res); }
private:
    void draw_batch();

    uint32_t program_id;
    uint32_t ebo_id;
    std::vector<sprite> sprites;

    class camera_manager {
    public:
        void set_attr_idx(uint32_t attr_idx) { cam_attr_idx = attr_idx; }
        void set_camera(vec2<float> pos);
        void set_resolution(vec2<uint16_t> res);

        void use_sprite_cam() { use_cam(sprite_mat.data()); }
        void use_ui_cam() { use_cam(ui_mat.data()); }
    private:
        void use_cam(float*);

        uint32_t cam_attr_idx = 0;
        vec2<float> res_scale;
        std::array<float, 16> sprite_mat = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,};
        std::array<float, 16> ui_mat;
    } camera;

    // Class to facilitate sprite batching
    struct batcher {
        batcher();

        size_t size();
        size_t capacity();

        // returns the number of bytes left unread
        size_t add(vertex* in, size_t num_verts);
        void lock();
        void release();
    private:
        vertex* buf = nullptr;
        size_t batched = 0;
        uint32_t vbo_id = 0;
        uint32_t vao_id = 0;
    } batch;

    class texture_manager {
    public:
        texture_manager();
        uint32_t add(uint8_t* buf, unsigned width, unsigned height);
        void use(size_t index);
    private:
        uint32_t gen_texture(uint8_t* buf, unsigned width, unsigned height);
        marked_vector<uint32_t> textures;
    } textures;
};

#endif //RENDERER_H
