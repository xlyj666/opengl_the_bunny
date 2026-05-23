#ifndef MATH3D_H
#define MATH3D_H

#include <cmath>
#include <cstring>

struct vec3 {
    float x, y, z;
    vec3() : x(0), y(0), z(0) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct mat4 {
    float m[16];

    mat4() { memset(m, 0, sizeof(m)); }

    static mat4 identity() {
        mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    float& operator[](int i) { return m[i]; }
    const float& operator[](int i) const { return m[i]; }

    mat4 operator*(const mat4& b) const {
        mat4 r;
        for (int row = 0; row < 4; row++)
            for (int col = 0; col < 4; col++)
                for (int k = 0; k < 4; k++)
                    r.m[row * 4 + col] += m[row * 4 + k] * b.m[k * 4 + col];
        return r;
    }
};

inline mat4 translate(float x, float y, float z) {
    mat4 r = mat4::identity();
    r.m[12] = x; r.m[13] = y; r.m[14] = z;
    return r;
}

inline mat4 scale(float x, float y, float z) {
    mat4 r;
    r.m[0] = x; r.m[5] = y; r.m[10] = z; r.m[15] = 1.0f;
    return r;
}

inline mat4 perspective(float fov, float aspect, float near, float far) {
    mat4 r;
    float f = 1.0f / tanf(fov * 3.14159265359f / 360.0f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = 2.0f * far * near / (near - far);
    return r;
}

inline vec3 normalize(const vec3& v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1e-8f) return v;
    return vec3(v.x / len, v.y / len, v.z / len);
}

inline vec3 cross(const vec3& a, const vec3& b) {
    return vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

inline vec3 subtract(const vec3& a, const vec3& b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline float dot(const vec3& a, const vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
    vec3 f = normalize(subtract(center, eye));
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    mat4 r = mat4::identity();
    r.m[0]  = s.x;   r.m[4]  = s.y;   r.m[8]  = s.z;
    r.m[1]  = u.x;   r.m[5]  = u.y;   r.m[9]  = u.z;
    r.m[2]  = -f.x;  r.m[6]  = -f.y;  r.m[10] = -f.z;
    r.m[12] = -dot(s, eye);
    r.m[13] = -dot(u, eye);
    r.m[14] = dot(f, eye);
    return r;
}

#endif
