#ifndef COORDINATE_TYPES_H
#define COORDINATE_TYPES_H

template <typename T>
struct vec2 {
    T x, y;
};

struct vertex {
    vec2<float> pos;
    vec2<float> uv;
};


#endif //COORDINATE_TYPES_H
