#include "renderer.h"
#include <GL/glew.h>
#include <cstring> //memcpy

void renderer::camera_manager::set_resolution(vec2<uint16_t> res) {
    glViewport(0, 0, res.x, res.y);
    sprite_mat[0] = 2.0f / float(res.x);
    sprite_mat[5] = -2.0f / float(res.y);

    ui_mat[0] = 2.0f / float(res.x);
    ui_mat[5] = -2.0f / float(res.y);

    // UI camera never moves, update translation values here instead
    ui_mat[12] = -1;
    ui_mat[13] = 1;
}

void renderer::camera_manager::set_camera(vec2<float> pos) {
    sprite_mat[12] = -1.0f + (pos.x * sprite_mat[0]);
    sprite_mat[13] = 1.0f +  (pos.y * sprite_mat[5]);
}

void renderer::camera_manager::use_cam(float* data) {
    glUniformMatrix4fv(cam_attr_idx, 1, GL_FALSE, data);
}
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
    

renderer::texture_manager::texture_manager() : textures(64) {};
uint32_t renderer::texture_manager::add(uint8_t* buf, unsigned width, unsigned height) {
    return textures.insert(gen_texture(buf, width, height));
}
void renderer::texture_manager::use(size_t index) {
    // it's possible to use this function to abstract away normal/height map binding
    // test if those aspects exist for the given texid, bind them to other texture units if so
     glBindTexture(GL_TEXTURE_2D, textures[index]);
}
uint32_t renderer::texture_manager::gen_texture(uint8_t* buf, unsigned width, unsigned height) {
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


// love 2 debug :D
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,const GLchar* message, const void* userParam ) {
    fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
                 ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ), type, severity, message );
}


renderer::batcher::batcher() {
    glGenBuffers(1, &vbo_id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    lock();
}

size_t renderer::batcher::size() { return batched; }
size_t renderer::batcher::capacity() { return 4096; }

// returns the number of bytes left unread
size_t renderer::batcher::add(vertex* in, size_t num_verts) {
    size_t verts_to_copy = std::min(num_verts, capacity() - batched);
    memcpy(buf + batched, in, verts_to_copy * sizeof(vertex));
    batched += verts_to_copy;
    return num_verts - verts_to_copy;
}

void renderer::batcher::lock() {
    glBufferData(GL_ARRAY_BUFFER, capacity() * sizeof(vertex), nullptr, GL_STREAM_DRAW);
    buf = (vertex*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    batched = 0;
}
void renderer::batcher::release() { glUnmapBuffer(GL_ARRAY_BUFFER); }


renderer::renderer() {
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    // feel it's best to load the shader first, maybe? no real reason
    program_id = assemble_program(vertshader, fragshader);
    glUseProgram(program_id);

    // element buffer :)
    std::array<uint16_t, 4096 * 6> index_buffer{};
    uint8_t lookup[] = {0, 1, 2, 2, 3, 0};
    for (size_t i = 0; i < 4096; i++) {
        index_buffer[i] = ((i / 6) * 4) + lookup[i % 6];
    }
    glGenBuffers(1, &ebo_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer.size() * sizeof(uint16_t), &index_buffer[0], GL_STATIC_DRAW);

    // note that XY and UV are two separate attribs, and that stride and offset accomodate interleaving
    // each attribute repeats every 4 floats, while the second attribute has an offset of 2
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));

    camera.set_attr_idx(glGetUniformLocation(program_id, "cam"));
    camera.use_sprite_cam();
}


void renderer::draw_batch() {
    batch.release();
    //glDrawArrays(GL_TRIANGLES, 0, 6);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*) 0);
    batch.lock();
}

void renderer::render() {
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

uint32_t renderer::add_texture(uint8_t* buf, unsigned w, unsigned h) { return textures.add(buf, w, h); }

void renderer::clear_sprites() { sprites.clear(); }
void renderer::add_sprite(const sprite& s) {
    sprites.emplace_back(s);
}



