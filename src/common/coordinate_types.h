#ifndef COORDINATE_TYPES_H
#define COORDINATE_TYPES_H

#include <stdint.h>
#include <math.h>

template <typename T>
struct vec2 {
    T x = 0, y = 0;
	template <typename U> vec2<U> to() { return vec2<U>{(U)x, (U)y}; }
};

template <typename T>
struct mat3 {
	static mat3<T> identity() { return {{1, 0, 0, 0, 1, 0, 0, 0, 1 }}; }
	static mat3<T> rotation(float deg, vec2<T> origin) {
		mat3<T> id = identity();
		float rad = deg * (M_PI / 180);
		id.data[0] = cos(rad);
		id.data[1] = -sin(rad);
		id.data[2] = (-origin.x * cos(rad)) + (origin.y * sin(rad)) + origin.x;
		id.data[3] = sin(rad);
		id.data[4] = cos(rad);
		id.data[5] = (-origin.x * sin(rad)) - (origin.y * cos(rad)) + origin.y;

		return id;
	}
	T data[9];
};

struct vertex {
    vec2<uint16_t> pos = vec2<uint16_t>{0, 0};
    vec2<float> uv;
};

// mat3 X mat3
/*template <typename T>
mat3<T> operator *(const mat3<T>& lhs, const mat3<T>& rhs) {
	mat3<T> temp;
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 3; x++) {
			T sum = 0;

			for (int i = 0; i < 3; i++) {
				sum += lhs
			}

			temp[x + (y * 3)] = val;
		}
	}
	temp.x0 = lhs.x0 * rhs.x0 + lhs.x1 * rhs.x2;
	temp.x1 = lhs.x0 * rhs.x1 + lhs.x1 * rhs.x3;
	temp.x2 = lhs.x2 * rhs.x0 + lhs.x2 * rhs.x2;
	temp.x3 = lhs.x2 * rhs.x1 + lhs.x2 * rhs.x3;
	return temp;
}*/

// mat3 X vec2 - adds a Z coordinate of 1.
template <typename T>
vec2<T> operator *(const mat3<T>& lhs, const vec2<T>& rhs) {
	vec2<T> temp;
	temp.x = lhs.data[0] * rhs.x + lhs.data[1] * rhs.y + lhs.data[2] * 1;
	temp.y = lhs.data[3] * rhs.x + lhs.data[4] * rhs.y + lhs.data[5] * 1;
	return temp;
}

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
