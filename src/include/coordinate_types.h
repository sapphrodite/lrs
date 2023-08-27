#ifndef COORDINATE_TYPES_H
#define COORDINATE_TYPES_H

#include <stdint.h>

template <typename T>
struct vec2 {
	T x = 0, y = 0;
	template <typename U> vec2<U> to() { return vec2<U>{(U)x, (U)y}; }
};

using vec2i = vec2<int>;
using vec2u = vec2<unsigned>;
using vec2f = vec2<float>;

struct vertex {
	vec2<uint16_t> pos;
	vec2<float> uv;
};

#define VEC2_ARITH(op) \
template <typename T> \
vec2<T> operator op (vec2<T> lhs, const vec2<T>& rhs) { \
	lhs op##= rhs; \
	return lhs; \
}

#define VEC2_COMPOUNDARITH(op) \
template <typename T> \
vec2<T> operator op##=(vec2<T>& lhs, const vec2<T>& rhs) { \
	lhs.x op##= rhs.x; \
	lhs.y op##= rhs.y; \
	return lhs; \
}

#define VEC2_DEFINE_OP(op) \
	VEC2_ARITH(op) \
	VEC2_COMPOUNDARITH(op)

VEC2_DEFINE_OP(+)
VEC2_DEFINE_OP(-)
VEC2_DEFINE_OP(*)
VEC2_DEFINE_OP(/)

#endif //COORDINATE_TYPES_H
