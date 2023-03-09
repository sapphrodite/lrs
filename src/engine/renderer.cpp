#include "modules.h"
#include <GL/glew.h>
#include <cstring> //memcpy

#include <array>
#include <common/coordinate_types.h>
#include <common/marked_array.h>




namespace renderer {

    // it's possible to use the "use" function to abstract away normal/height map binding
    // test if those aspects exist for the given texid, bind them to other texture units if so
    class texture_manager {
    public:
        texture_manager() : textures(64) {};
        uint32_t add(const uint8_t* buf, unsigned w, unsigned h) { return textures.insert_any(texgen(buf, w, h)); }
        void use(size_t index) { glBindTexture(GL_TEXTURE_2D, textures[index]); }
    private:
        marked_vector<uint32_t> textures;

        uint32_t texgen(const uint8_t* buf, unsigned width, unsigned height) {
            // generate a texture object, and set some properties
            // we want to clamp sampling to not wrap-around, and use nearest-neighbor sampling
            uint32_t texid = 0;
            glGenTextures(1, &texid);
            glBindTexture(GL_TEXTURE_2D, texid);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            // this call is a fucking whirlwind, good luck!
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);
            return texid;
        }
    };


    // Class to facilitate sprite batching
    struct batcher {
        batcher() {
            glGenBuffers(1, &vbo_id);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
            lock();
        }

        size_t size() { return batched; }
        size_t capacity() { return 4096; }

        // returns the number of vertices left unread
        size_t add(vertex* in, size_t num_verts) {
            size_t verts_to_copy = std::min(num_verts, capacity() - batched);
            memcpy(buf + batched, in, verts_to_copy * sizeof(vertex));
            batched += verts_to_copy;
            return num_verts - verts_to_copy;
        }
        void lock()  {
            glBufferData(GL_ARRAY_BUFFER, capacity() * sizeof(vertex), nullptr, GL_STREAM_DRAW);
            buf = (vertex*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            batched = 0;
        }
        void release() { glUnmapBuffer(GL_ARRAY_BUFFER); }
    private:
        vertex* buf = nullptr;
        size_t batched = 0;
        uint32_t vbo_id = 0;
        uint32_t vao_id = 0;
    };


    class camera_manager {
    public:
        void set_attr_idx(uint32_t attr_idx) { cam_attr_idx = attr_idx; }
        void set_camera(vec2<float> pos) {
            sprite_mat[12] = -1.0f + (pos.x * sprite_mat[0]);
            sprite_mat[13] = 1.0f +  (pos.y * sprite_mat[5]);
        }
        void set_res(vec2<uint16_t> res) {
            glViewport(0, 0, res.x, res.y);
            sprite_mat[0] = 2.0f / float(res.x);
            sprite_mat[5] = -2.0f / float(res.y);

            ui_mat[0] = 2.0f / float(res.x);
            ui_mat[5] = -2.0f / float(res.y);

            // UI camera never moves, update translation values here instead
            ui_mat[12] = -1;
            ui_mat[13] = 1;
        }

        void use_sprite_cam() { use_cam(sprite_mat.data()); }
        void use_ui_cam() { use_cam(ui_mat.data()); }
    private:
        void use_cam(float* data) {
            glUniformMatrix4fv(cam_attr_idx, 1, GL_FALSE, data);
        }

        uint32_t cam_attr_idx = 0;
        vec2<float> res_scale;
        std::array<float, 16> sprite_mat = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,};
        std::array<float, 16> ui_mat;
    };


class handle {
public:
    handle();
    void render();

    void draw_batch();

    uint32_t program_id;
    uint32_t ebo_id;
    std::vector<sprite> sprites;

    camera_manager camera;
    batcher batch;
    texture_manager textures;
};











    handle* alloc() { return new handle; }
    void free(handle* r) { delete r; }
    void draw(handle* r) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        r->render();
    }
    void glew_init() { glewInit(); }

    uint32_t add_tex(handle* r, const uint8_t* buf, unsigned w, unsigned h) { return r->textures.add(buf, w, h); }
    void add_sprite(handle* r, const sprite& s) { r->sprites.emplace_back(s); }
    void clear_sprites(handle* r) { r->sprites.clear(); }

    void set_cam(handle* r, float cam_x, float cam_y) { r->camera.set_camera(vec2<float>(cam_x, cam_y)); }
    void set_res(handle* r, uint16_t cam_x, uint16_t cam_y) { r->camera.set_res(vec2<uint16_t>(cam_x, cam_y)); }









// a shader here, refers only to a single-stage object!
// regardless, this function takes in shader code and returns an ID associated w it :)
uint32_t compile_shader(GLenum type, const char* shader_code) {
    uint32_t ID = glCreateShader(type);
    glShaderSource(ID, 1, &shader_code, 0);
    glCompileShader(ID);
    return ID;
}


// OpenGL calls a fully assembled GPU shader pipeline a "program object"
// Program objects represent the majority of actual code executed on a GPU.
// Every other OpenGL call typically manages data or state or other misc. things
// welcome to overly verbose hell :)
uint32_t assemble_program(const char* vertcode, const char* fragcode) {
    uint32_t vertID = compile_shader(GL_VERTEX_SHADER, vertcode);
    uint32_t fragID = compile_shader(GL_FRAGMENT_SHADER, fragcode);
    uint32_t progID = glCreateProgram();
    glAttachShader(progID, vertID);
    glAttachShader(progID, fragID);
    glLinkProgram(progID);
    glDetachShader(progID, vertID);
    glDetachShader(progID, fragID);
    glDeleteShader(vertID);
    glDeleteShader(fragID);
    return progID;
}


const char* vertshader =
"#version 330 core\n"

"layout(location = 0) in vec2 vertexPosition_modelspace;\n"
"layout(location = 1) in vec2 vertexUV;\n"
"out vec2 UV;\n"

"uniform mat4 cam;\n"

"void main(){\n"
    "gl_Position = cam * vec4(vertexPosition_modelspace, 0, 1);\n"
    "UV = vertexUV;\n"
"}\n";

const char* fragshader =
"#version 330 core\n"

"in vec2 UV;\n"
"out vec3 color;\n"
"uniform sampler2D myTextureSampler;\n"

"void main(){\n"
    "color = texture( myTextureSampler, UV ).rgb;\n"
"}\n";
    



// love 2 debug :D
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,const GLchar* message, const void* userParam ) {
    fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
                 ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ), type, severity, message );
}




handle::handle() {
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    // feel it's best to load the shader first, maybe? no real reason
    program_id = assemble_program(vertshader, fragshader);
    glUseProgram(program_id);

    // element buffer :)
    std::array<uint16_t, 4096 * 6> index_buffer{};
    uint8_t lookup[] = {0, 1, 3, 3, 2, 0};
    for (size_t i = 0; i < 4096; i++) {
        index_buffer[i] = ((i / 6) * 4) + lookup[i % 6];
    }
    glGenBuffers(1, &ebo_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer.size() * sizeof(uint16_t), &index_buffer[0], GL_STATIC_DRAW);

    // note that XY and UV are two separate attribs, and that stride and offset accomodate interleaving
    // each attribute repeats every sizeof(vertex), while the second attribute has an offset from the first in memory
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(vertex), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*) offsetof(vertex, uv));

    camera.set_attr_idx(glGetUniformLocation(program_id, "cam"));
    camera.use_sprite_cam();
}


void handle::draw_batch() {
    batch.release();
    //glDrawArrays(GL_TRIANGLES, 0, 6);
    glDrawElements(GL_TRIANGLES, (batch.size() / 4) * 6, GL_UNSIGNED_SHORT, (void*) 0);
    batch.lock();
}

void handle::render() {
    camera.use_sprite_cam();
    for (auto& sprite : sprites) {
        // if textures are different, draw batch now

        size_t total_batched = 0;
        while (total_batched < sprite.data.size()) {
            size_t to_batch = sprite.data.size() - total_batched;
            size_t remaining = batch.add(sprite.data.data() + total_batched, to_batch);
            total_batched += to_batch - remaining;

            if (remaining > 0) {
                draw_batch();
            }

        }
    }
    draw_batch();
}





}




