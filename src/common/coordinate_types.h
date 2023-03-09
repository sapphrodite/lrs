#ifndef COORDINATE_TYPES_H
#define COORDINATE_TYPES_H

template <typename T>
struct vec2 {
    T x, y;
};

struct vertex {
    vec2<uint16_t> pos = vec2<uint16_t>{0, 0};
    vec2<float> uv;
};


#endif //COORDINATE_TYPES_H
