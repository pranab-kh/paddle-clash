#ifndef MATH3D_H
#define MATH3D_H

#include <cmath> //used sqrtf, cosf, sinf, tanf

struct Vec3 {
    float x, y, z;
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};


inline Vec3 operator+(const Vec3& a, const Vec3& b) {
    return {a.x+b.x, a.y+b.y, a.z+b.z};
}


inline Vec3 operator-(const Vec3& a, const Vec3& b) {
    return {a.x-b.x, a.y-b.y, a.z-b.z};
}


inline Vec3 operator*(const Vec3& v, float s) {
    return {v.x*s, v.y*s, v.z*s};
}


inline float dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}


inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}


inline float magnitude(const Vec3& v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}


inline Vec3 normalize(const Vec3& v) {
    float m = magnitude(v);
    return {v.x/m, v.y/m, v.z/m};
}


struct Vec4 {
    float x, y, z, w;
    Vec4(float x = 0, float y = 0, float z = 0, float w = 0) : x(x), y(y), z(z), w(w) {}
};


struct Mat4 {
    float m[16]; // 16 floats, 4x4 matrix stored column-major

    Mat4() {
        for(int i = 0; i < 16; i++) m[i] = 0.0f;
    }

    // access element at row r, column c
    float& at(int r, int c)       { return m[c*4 + r]; }
    float  at(int r, int c) const { return m[c*4 + r]; }
};


inline Mat4 operator*(const Mat4& A, const Mat4& B) {
    Mat4 C;
    for(int r = 0; r < 4; r++) {
        for(int c = 0; c < 4; c++) {
            float sum = 0;
            for(int k = 0; k < 4; k++)
                sum += A.at(r, k) * B.at(k, c);
            C.at(r, c) = sum;
        }
    }
    return C;
}


inline Mat4 identity() {
    Mat4 M;
    M.at(0,0) = 1.0f;
    M.at(1,1) = 1.0f;
    M.at(2,2) = 1.0f;
    M.at(3,3) = 1.0f;
    return M;
}


inline Mat4 translate(float tx, float ty, float tz) {
    Mat4 M = identity();
    M.at(0,3) = tx;
    M.at(1,3) = ty;
    M.at(2,3) = tz;
    return M;
}


inline Mat4 scale(float sx, float sy, float sz) {
    Mat4 M = identity();
    M.at(0,0) = sx;
    M.at(1,1) = sy;
    M.at(2,2) = sz;
    return M;
}


inline Mat4 rotateX(float angle) {
    Mat4 M = identity();
    float c = cosf(angle), s = sinf(angle);
    M.at(1,1) =  c;
    M.at(1,2) = -s;
    M.at(2,1) =  s;
    M.at(2,2) =  c;
    return M;
}

inline Mat4 rotateY(float angle) {
    Mat4 M = identity();
    float c = cosf(angle), s = sinf(angle);
    M.at(0,0) =  c;
    M.at(0,2) =  s;
    M.at(2,0) = -s;
    M.at(2,2) =  c;
    return M;
}

inline Mat4 rotateZ(float angle) {
    Mat4 M = identity();
    float c = cosf(angle), s = sinf(angle);
    M.at(0,0) =  c;
    M.at(0,1) = -s;
    M.at(1,0) =  s;
    M.at(1,1) =  c;
    return M;
}

inline float toRadians(float degrees) {
    return degrees * (3.14159265358979f / 180.0f);
}


inline Mat4 perspective(float fovDeg, float aspect, float near, float far) {
    //Parameters:
    // fovDeg  — field of view angle: width of vision cone 
    //           90° = wide, fish-eye feel
    //           45° = narrow, zoomed in feel

    // aspect  — width/height of screen, 16:9 screen = 1.777     
    // near    — closest distance you can see
    // far     — furthest distance you can see.

    Mat4 M; 
    float fov = toRadians(fovDeg);
    float f = 1.0f / tanf(fov / 2.0f); // focal length

    M.at(0,0) = f / aspect;
    M.at(1,1) = f;
    M.at(2,2) = (far + near) / (near - far);
    M.at(2,3) = (2.0f * far * near) / (near - far);
    M.at(3,2) = -1.0f;

    return M;
}


inline Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    // eye = where the camera is, target = what its looking at, up= which way is up (0,1,0)         
    Vec3 f = normalize(target - eye);   // forward: direction camera looks at
    Vec3 r = normalize(cross(f, up));   // right: perpendicular to forward and up
    Vec3 u = cross(r, f);               // true up: perpendicular to both

    Mat4 M = identity();

    // rotation part
    M.at(0,0) =  r.x;  M.at(0,1) =  r.y;  M.at(0,2) =  r.z;
    M.at(1,0) =  u.x;  M.at(1,1) =  u.y;  M.at(1,2) =  u.z;
    M.at(2,0) = -f.x;  M.at(2,1) = -f.y;  M.at(2,2) = -f.z;

    // translation part
    M.at(0,3) = -dot(r, eye);
    M.at(1,3) = -dot(u, eye);
    M.at(2,3) =  dot(f, eye);

    return M;
}


#endif