
#pragma once

#include <algorithm>
#include <cmath>
#include <ostream>

#include "log.h"
#include "vec4.h"

struct Mat4 {

    /// Identity matrix
    static const Mat4 I;
    /// Zero matrix
    static const Mat4 Zero;

    /// Return transposed matrix
    static Mat4 transpose(const Mat4& m);
    /// Return inverse matrix (will be NaN if m is not invertible)
    static Mat4 inverse(const Mat4& m);
    /// Return transformation matrix for given translation vector
    static Mat4 translate(Vec3 t);
    /// Return transformation matrix for given angle (degrees) and axis
    static Mat4 rotate(float t, Vec3 axis);
    /// Return transformation matrix for given XYZ Euler angle rotation
    static Mat4 euler(Vec3 angles);
    /// Return transformation matrix that rotates the Y axis to dir
    static Mat4 rotate_to(Vec3 dir);
    /// Return transformation matrix that rotates the -Z axis to dir
    static Mat4 rotate_z_to(Vec3 dir);
    /// Return transformation matrix for given scale factors
    static Mat4 scale(Vec3 s);
    /// Return transformation matrix with given axes
    static Mat4 axes(Vec3 x, Vec3 y, Vec3 z);

    /// Return transformation matrix for viewing a scene from $pos looking at $at,
    /// where straight up is defined as $up
    static Mat4 look_at(Vec3 pos, Vec3 at, Vec3 up = Vec3{0.0f, 1.0f, 0.0f});
    /// Return orthogonal projection matrix with given left, right, bottom, top,
    /// near, and far planes.
    static Mat4 ortho(float l, float r, float b, float t, float n, float f);

    /// Return perspective projection matrix with given field of view, aspect ratio,
    /// and near plane. The far plane is assumed to be at infinity. This projection
    /// also outputs n/z for better precision with floating point depth buffers, so we use
    /// a depth mapping where 0 is the far plane (infinity) and 1 is the near plane, and
    /// an object is closer if is depth is greater.
    static Mat4 project(float fov, float ar, float n);

    Mat4() {
        *this = I;
    }
    explicit Mat4(Vec4 x, Vec4 y, Vec4 z, Vec4 w) {
        cols[0] = x;
        cols[1] = y;
        cols[2] = z;
        cols[3] = w;
    }

    Mat4(const Mat4&) = default;
    Mat4& operator=(const Mat4&) = default;
    ~Mat4() = default;

    Vec4& operator[](int idx) {
        assert(idx >= 0 && idx <= 3);
        return cols[idx];
    }
    Vec4 operator[](int idx) const {
        assert(idx >= 0 && idx <= 3);
        return cols[idx];
    }

    Mat4 operator+=(const Mat4& m) {
        for(int i = 0; i < 4; i++) cols[i] += m.cols[i];
        return *this;
    }
    Mat4 operator-=(const Mat4& m) {
        for(int i = 0; i < 4; i++) cols[i] -= m.cols[i];
        return *this;
    }

    Mat4 operator+=(float s) {
        for(int i = 0; i < 4; i++) cols[i] += s;
        return *this;
    }
    Mat4 operator-=(float s) {
        for(int i = 0; i < 4; i++) cols[i] -= s;
        return *this;
    }
    Mat4 operator*=(float s) {
        for(int i = 0; i < 4; i++) cols[i] *= s;
        return *this;
    }
    Mat4 operator/=(float s) {
        for(int i = 0; i < 4; i++) cols[i] /= s;
        return *this;
    }

    Mat4 operator+(const Mat4& m) const {
        Mat4 r;
        for(int i = 0; i < 4; i++) r.cols[i] = cols[i] + m.cols[i];
        return r;
    }
    Mat4 operator-(const Mat4& m) const {
        Mat4 r;
        for(int i = 0; i < 4; i++) r.cols[i] = cols[i] - m.cols[i];
        return r;
    }

    Mat4 operator+(float s) const {
        Mat4 r;
        for(int i = 0; i < 4; i++) r.cols[i] = cols[i] + s;
        return r;
    }
    Mat4 operator-(float s) const {
        Mat4 r;
        for(int i = 0; i < 4; i++) r.cols[i] = cols[i] - s;
        return r;
    }
    Mat4 operator*(float s) const {
        Mat4 r;
        for(int i = 0; i < 4; i++) r.cols[i] = cols[i] * s;
        return r;
    }
    Mat4 operator/(float s) const {
        Mat4 r;
        for(int i = 0; i < 4; i++) r.cols[i] = cols[i] / s;
        return r;
    }

    Mat4 operator*=(const Mat4& v) {
        *this = *this * v;
        return *this;
    }
    Mat4 operator*(const Mat4& m) const {
        Mat4 ret;
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                ret[i][j] = 0.0f;
                for(int k = 0; k < 4; k++) {
                    ret[i][j] += m[i][k] * cols[k][j];
                }
            }
        }
        return ret;
    }

    Vec4 operator*(Vec4 v) const {
        return v[0] * cols[0] + v[1] * cols[1] + v[2] * cols[2] + v[3] * cols[3];
    }

    /// Expands v to Vec4(v, 1.0), multiplies, and projects back to 3D
    Vec3 operator*(Vec3 v) const {
        return operator*(Vec4(v, 1.0f)).project();
    }
    /// Expands v to Vec4(v, 0.0), multiplies, and projects back to 3D
    Vec3 rotate(Vec3 v) const {
        return operator*(Vec4(v, 0.0f)).xyz();
    }

    /// Converts rotation (orthonormal 3x3) matrix to equivalent Euler angles
    Vec3 to_euler() const {

        bool single = true;
        static const float singularity[] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
                                            0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0};
        for(int i = 0; i < 12 && single; i++) {
            single = single && std::abs(data[i] - singularity[i]) < 16.0f * FLT_EPSILON;
        }
        if(single) return Vec3{0.0f, 0.0f, 180.0f};

        Vec3 eul1, eul2;

        float cy = std::hypotf(cols[0][0], cols[0][1]);
        if(cy > 16.0f * FLT_EPSILON) {
            eul1[0] = std::atan2(cols[1][2], cols[2][2]);
            eul1[1] = std::atan2(-cols[0][2], cy);
            eul1[2] = std::atan2(cols[0][1], cols[0][0]);

            eul2[0] = std::atan2(-cols[1][2], -cols[2][2]);
            eul2[1] = std::atan2(-cols[0][2], -cy);
            eul2[2] = std::atan2(-cols[0][1], -cols[0][0]);
        } else {
            eul1[0] = std::atan2(-cols[2][1], cols[1][1]);
            eul1[1] = std::atan2(-cols[0][2], cy);
            eul1[2] = 0;
            eul2 = eul1;
        }
        float d1 = std::abs(eul1[0]) + std::abs(eul1[1]) + std::abs(eul1[2]);
        float d2 = std::abs(eul2[0]) + std::abs(eul2[1]) + std::abs(eul2[2]);
        if(d1 > d2)
            return Degrees(eul2);
        else
            return Degrees(eul1);
    }

    /// Returns matrix transpose
    Mat4 T() const {
        return transpose(*this);
    }
    /// Returns matrix inverse (will be NaN if m is not invertible)
    Mat4 inverse() const {
        return inverse(*this);
    }

    /// Returns determinant (brute force).
    float det() const {
        return cols[0][3] * cols[1][2] * cols[2][1] * cols[3][0] -
               cols[0][2] * cols[1][3] * cols[2][1] * cols[3][0] -
               cols[0][3] * cols[1][1] * cols[2][2] * cols[3][0] +
               cols[0][1] * cols[1][3] * cols[2][2] * cols[3][0] +
               cols[0][2] * cols[1][1] * cols[2][3] * cols[3][0] -
               cols[0][1] * cols[1][2] * cols[2][3] * cols[3][0] -
               cols[0][3] * cols[1][2] * cols[2][0] * cols[3][1] +
               cols[0][2] * cols[1][3] * cols[2][0] * cols[3][1] +
               cols[0][3] * cols[1][0] * cols[2][2] * cols[3][1] -
               cols[0][0] * cols[1][3] * cols[2][2] * cols[3][1] -
               cols[0][2] * cols[1][0] * cols[2][3] * cols[3][1] +
               cols[0][0] * cols[1][2] * cols[2][3] * cols[3][1] +
               cols[0][3] * cols[1][1] * cols[2][0] * cols[3][2] -
               cols[0][1] * cols[1][3] * cols[2][0] * cols[3][2] -
               cols[0][3] * cols[1][0] * cols[2][1] * cols[3][2] +
               cols[0][0] * cols[1][3] * cols[2][1] * cols[3][2] +
               cols[0][1] * cols[1][0] * cols[2][3] * cols[3][2] -
               cols[0][0] * cols[1][1] * cols[2][3] * cols[3][2] -
               cols[0][2] * cols[1][1] * cols[2][0] * cols[3][3] +
               cols[0][1] * cols[1][2] * cols[2][0] * cols[3][3] +
               cols[0][2] * cols[1][0] * cols[2][1] * cols[3][3] -
               cols[0][0] * cols[1][2] * cols[2][1] * cols[3][3] -
               cols[0][1] * cols[1][0] * cols[2][2] * cols[3][3] +
               cols[0][0] * cols[1][1] * cols[2][2] * cols[3][3];
    }

    union {
        Vec4 cols[4];
        float data[16] = {};
    };
};

inline bool operator==(const Mat4& l, const Mat4& r) {
    for(int i = 0; i < 16; i++)
        if(l.data[i] != r.data[i]) return false;
    return true;
}

inline bool operator!=(const Mat4& l, const Mat4& r) {
    for(int i = 0; i < 16; i++)
        if(l.data[i] != r.data[i]) return true;
    return false;
}

inline Mat4 operator+(float s, const Mat4& m) {
    Mat4 r;
    for(int i = 0; i < 4; i++) r.cols[i] = m.cols[i] + s;
    return r;
}
inline Mat4 operator-(float s, const Mat4& m) {
    Mat4 r;
    for(int i = 0; i < 4; i++) r.cols[i] = m.cols[i] - s;
    return r;
}
inline Mat4 operator*(float s, const Mat4& m) {
    Mat4 r;
    for(int i = 0; i < 4; i++) r.cols[i] = m.cols[i] * s;
    return r;
}
inline Mat4 operator/(float s, const Mat4& m) {
    Mat4 r;
    for(int i = 0; i < 4; i++) r.cols[i] = m.cols[i] / s;
    return r;
}

const inline Mat4 Mat4::I = Mat4{Vec4{1.0f, 0.0f, 0.0f, 0.0f}, Vec4{0.0f, 1.0f, 0.0f, 0.0f},
                                 Vec4{0.0f, 0.0f, 1.0f, 0.0f}, Vec4{0.0f, 0.0f, 0.0f, 1.0f}};
const inline Mat4 Mat4::Zero = Mat4{Vec4{0.0f, 0.0f, 0.0f, 0.0f}, Vec4{0.0f, 0.0f, 0.0f, 0.0f},
                                    Vec4{0.0f, 0.0f, 0.0f, 0.0f}, Vec4{0.0f, 0.0f, 0.0f, 0.0f}};

inline Mat4 outer(Vec4 u, Vec4 v) {
    Mat4 B;
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++) B[i][j] = u[i] * v[j];
    return B;
}

inline Mat4 Mat4::transpose(const Mat4& m) {
    Mat4 r;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            r[i][j] = m[j][i];
        }
    }
    return r;
}

inline Mat4 Mat4::inverse(const Mat4& m) {
    Mat4 r;
    r[0][0] = m[1][2] * m[2][3] * m[3][1] - m[1][3] * m[2][2] * m[3][1] +
              m[1][3] * m[2][1] * m[3][2] - m[1][1] * m[2][3] * m[3][2] -
              m[1][2] * m[2][1] * m[3][3] + m[1][1] * m[2][2] * m[3][3];
    r[0][1] = m[0][3] * m[2][2] * m[3][1] - m[0][2] * m[2][3] * m[3][1] -
              m[0][3] * m[2][1] * m[3][2] + m[0][1] * m[2][3] * m[3][2] +
              m[0][2] * m[2][1] * m[3][3] - m[0][1] * m[2][2] * m[3][3];
    r[0][2] = m[0][2] * m[1][3] * m[3][1] - m[0][3] * m[1][2] * m[3][1] +
              m[0][3] * m[1][1] * m[3][2] - m[0][1] * m[1][3] * m[3][2] -
              m[0][2] * m[1][1] * m[3][3] + m[0][1] * m[1][2] * m[3][3];
    r[0][3] = m[0][3] * m[1][2] * m[2][1] - m[0][2] * m[1][3] * m[2][1] -
              m[0][3] * m[1][1] * m[2][2] + m[0][1] * m[1][3] * m[2][2] +
              m[0][2] * m[1][1] * m[2][3] - m[0][1] * m[1][2] * m[2][3];
    r[1][0] = m[1][3] * m[2][2] * m[3][0] - m[1][2] * m[2][3] * m[3][0] -
              m[1][3] * m[2][0] * m[3][2] + m[1][0] * m[2][3] * m[3][2] +
              m[1][2] * m[2][0] * m[3][3] - m[1][0] * m[2][2] * m[3][3];
    r[1][1] = m[0][2] * m[2][3] * m[3][0] - m[0][3] * m[2][2] * m[3][0] +
              m[0][3] * m[2][0] * m[3][2] - m[0][0] * m[2][3] * m[3][2] -
              m[0][2] * m[2][0] * m[3][3] + m[0][0] * m[2][2] * m[3][3];
    r[1][2] = m[0][3] * m[1][2] * m[3][0] - m[0][2] * m[1][3] * m[3][0] -
              m[0][3] * m[1][0] * m[3][2] + m[0][0] * m[1][3] * m[3][2] +
              m[0][2] * m[1][0] * m[3][3] - m[0][0] * m[1][2] * m[3][3];
    r[1][3] = m[0][2] * m[1][3] * m[2][0] - m[0][3] * m[1][2] * m[2][0] +
              m[0][3] * m[1][0] * m[2][2] - m[0][0] * m[1][3] * m[2][2] -
              m[0][2] * m[1][0] * m[2][3] + m[0][0] * m[1][2] * m[2][3];
    r[2][0] = m[1][1] * m[2][3] * m[3][0] - m[1][3] * m[2][1] * m[3][0] +
              m[1][3] * m[2][0] * m[3][1] - m[1][0] * m[2][3] * m[3][1] -
              m[1][1] * m[2][0] * m[3][3] + m[1][0] * m[2][1] * m[3][3];
    r[2][1] = m[0][3] * m[2][1] * m[3][0] - m[0][1] * m[2][3] * m[3][0] -
              m[0][3] * m[2][0] * m[3][1] + m[0][0] * m[2][3] * m[3][1] +
              m[0][1] * m[2][0] * m[3][3] - m[0][0] * m[2][1] * m[3][3];
    r[2][2] = m[0][1] * m[1][3] * m[3][0] - m[0][3] * m[1][1] * m[3][0] +
              m[0][3] * m[1][0] * m[3][1] - m[0][0] * m[1][3] * m[3][1] -
              m[0][1] * m[1][0] * m[3][3] + m[0][0] * m[1][1] * m[3][3];
    r[2][3] = m[0][3] * m[1][1] * m[2][0] - m[0][1] * m[1][3] * m[2][0] -
              m[0][3] * m[1][0] * m[2][1] + m[0][0] * m[1][3] * m[2][1] +
              m[0][1] * m[1][0] * m[2][3] - m[0][0] * m[1][1] * m[2][3];
    r[3][0] = m[1][2] * m[2][1] * m[3][0] - m[1][1] * m[2][2] * m[3][0] -
              m[1][2] * m[2][0] * m[3][1] + m[1][0] * m[2][2] * m[3][1] +
              m[1][1] * m[2][0] * m[3][2] - m[1][0] * m[2][1] * m[3][2];
    r[3][1] = m[0][1] * m[2][2] * m[3][0] - m[0][2] * m[2][1] * m[3][0] +
              m[0][2] * m[2][0] * m[3][1] - m[0][0] * m[2][2] * m[3][1] -
              m[0][1] * m[2][0] * m[3][2] + m[0][0] * m[2][1] * m[3][2];
    r[3][2] = m[0][2] * m[1][1] * m[3][0] - m[0][1] * m[1][2] * m[3][0] -
              m[0][2] * m[1][0] * m[3][1] + m[0][0] * m[1][2] * m[3][1] +
              m[0][1] * m[1][0] * m[3][2] - m[0][0] * m[1][1] * m[3][2];
    r[3][3] = m[0][1] * m[1][2] * m[2][0] - m[0][2] * m[1][1] * m[2][0] +
              m[0][2] * m[1][0] * m[2][1] - m[0][0] * m[1][2] * m[2][1] -
              m[0][1] * m[1][0] * m[2][2] + m[0][0] * m[1][1] * m[2][2];
    r /= m.det();
    return r;
}

inline Mat4 Mat4::rotate_to(Vec3 dir) {

    dir.normalize();

    if(std::abs(dir.y - 1.0f) < EPS_F)
        return Mat4::I;
    else if(std::abs(dir.y + 1.0f) < EPS_F)
        return Mat4{Vec4{1.0f, 0.0f, 0.0f, 0.0f}, Vec4{0.0f, -1.0f, 0.0f, 0.0f},
                    Vec4{0.0f, 0.0f, 1.0f, 0.0}, Vec4{0.0f, 0.0f, 0.0f, 1.0f}};
    else {
        Vec3 x = cross(dir, Vec3{0.0f, 1.0f, 0.0f}).unit();
        Vec3 z = cross(x, dir).unit();
        return Mat4{Vec4{x, 0.0f}, Vec4{dir, 0.0f}, Vec4{z, 0.0f}, Vec4{0.0f, 0.0f, 0.0f, 1.0f}};
    }
}

inline Mat4 Mat4::rotate_z_to(Vec3 dir) {
    Mat4 y = rotate_to(dir);
    Vec4 _y = y[1];
    Vec4 _z = y[2];
    y[1] = _z;
    y[2] = -_y;
    return y;
}

inline Mat4 Mat4::axes(Vec3 x, Vec3 y, Vec3 z) {
    return Mat4{Vec4{x, 0.0f}, Vec4{y, 0.0f}, Vec4{z, 0.0f}, Vec4{0.0f, 0.0f, 0.0f, 1.0f}};
}

inline Mat4 Mat4::translate(Vec3 t) {
    Mat4 r;
    r[3] = Vec4(t, 1.0f);
    return r;
}

inline Mat4 Mat4::euler(Vec3 angles) {
    return Mat4::rotate(angles.z, Vec3{0.0f, 0.0f, 1.0f}) *
           Mat4::rotate(angles.y, Vec3{0.0f, 1.0f, 0.0f}) *
           Mat4::rotate(angles.x, Vec3{1.0f, 0.0f, 0.0f});
}

inline Mat4 Mat4::rotate(float t, Vec3 axis) {
    Mat4 ret;
    float c = std::cos(Radians(t));
    float s = std::sin(Radians(t));
    axis.normalize();
    Vec3 temp = axis * (1.0f - c);
    ret[0][0] = c + temp[0] * axis[0];
    ret[0][1] = temp[0] * axis[1] + s * axis[2];
    ret[0][2] = temp[0] * axis[2] - s * axis[1];
    ret[1][0] = temp[1] * axis[0] - s * axis[2];
    ret[1][1] = c + temp[1] * axis[1];
    ret[1][2] = temp[1] * axis[2] + s * axis[0];
    ret[2][0] = temp[2] * axis[0] + s * axis[1];
    ret[2][1] = temp[2] * axis[1] - s * axis[0];
    ret[2][2] = c + temp[2] * axis[2];
    return ret;
}

inline Mat4 Mat4::scale(Vec3 s) {
    Mat4 r;
    r[0][0] = s.x;
    r[1][1] = s.y;
    r[2][2] = s.z;
    return r;
}

inline Mat4 Mat4::ortho(float l, float r, float b, float t, float n, float f) {
    Mat4 rs;
    rs[0][0] = 2.0f / (r - l);
    rs[1][1] = 2.0f / (t - b);
    rs[2][2] = 2.0f / (n - f);
    rs[3][0] = (-l - r) / (r - l);
    rs[3][1] = (-b - t) / (t - b);
    rs[3][2] = -n / (f - n);
    return rs;
}

inline Mat4 Mat4::project(float fov, float ar, float n) {
    float f = 1.0f / std::tan(Radians(fov) / 2.0f);
    Mat4 r;
    r[0][0] = f / ar;
    r[1][1] = f;
    r[2][2] = 0.0f;
    r[3][3] = 0.0f;
    r[3][2] = n;
    r[2][3] = -1.0f;
    return r;
}

inline Mat4 Mat4::look_at(Vec3 pos, Vec3 at, Vec3 up) {
    Mat4 r = Mat4::Zero;
    Vec3 F = (at - pos).unit();
    Vec3 S = cross(F, up).unit();
    Vec3 U = cross(S, F).unit();
    r[0][0] = S.x;
    r[0][1] = U.x;
    r[0][2] = -F.x;
    r[1][0] = S.y;
    r[1][1] = U.y;
    r[1][2] = -F.y;
    r[2][0] = S.z;
    r[2][1] = U.z;
    r[2][2] = -F.z;
    r[3][0] = -dot(S, pos);
    r[3][1] = -dot(U, pos);
    r[3][2] = dot(F, pos);
    r[3][3] = 1.0f;
    return r;
}

inline std::ostream& operator<<(std::ostream& out, Mat4 m) {
    out << "{" << m[0] << "," << m[1] << "," << m[2] << "," << m[3] << "}";
    return out;
}
