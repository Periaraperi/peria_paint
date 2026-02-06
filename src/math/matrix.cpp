#include "math/matrix.hpp"
#include <cmath>

namespace peria::math {

mat4f get_ortho_projection(float left, float right, float bottom, float top) noexcept
{
    mat4f m{0.0f};
    m(0, 0) = 2.0f / (right - left);
    m(0, 1) = 0.0f;
    m(0, 2) = 0.0f;
    m(0, 3) = -(right + left) / (right - left);
                                                
    m(1, 0) = 0.0f;
    m(1, 1) = 2.0f / (top - bottom);
    m(1, 2) = 0.0f;
    m(1, 3) = -(top + bottom) / (top - bottom);
                                                
    m(2, 0) = 0.0f;
    m(2, 1) = 0.0f;
    m(2, 2) = 1.0f;
    m(2, 3) = 0.0f;
                   
    m(3, 0) = 0.0f;
    m(3, 1) = 0.0f;
    m(3, 2) = 0.0f;
    m(3, 3) = 1.0f;

    return m;
}

mat4f translate(float x, float y, float z) noexcept
{
    mat4f m{MAT4F_IDENTITY};
    m(0, 3) = x;
    m(1, 3) = y;
    m(2, 3) = z; 
    return m; 
}

mat4f scale(float x, float y, float z) noexcept
{
    mat4f m{MAT4F_IDENTITY};
    m(0, 0) = x;
    m(1, 1) = y;
    m(2, 2) = z;
    return m;
}

mat4f rotate(float angle_x /*in radians*/, float angle_y, float angle_z) noexcept
{
    mat4f m{MAT4F_IDENTITY};
    if (angle_x != 0.0f) {
        mat4f rot_x{};
        rot_x(1, 1) = std::cos(angle_x);
        rot_x(1, 2) = -std::sin(angle_x);
        rot_x(2, 1) = std::sin(angle_x);
        rot_x(2, 2) = std::cos(angle_x);
        m = rot_x * m;
    }
    if (angle_y != 0.0f) {
        mat4f rot_y{};
        rot_y(0, 0) = std::cos(angle_y);
        rot_y(0, 2) = std::sin(angle_y);
        rot_y(2, 0) = -std::sin(angle_y);
        rot_y(2, 2) = std::cos(angle_y);
        m = rot_y * m;
    }
    if (angle_z != 0.0f) {
        mat4f rot_z{};
        rot_z(0, 0) = std::cos(angle_z);
        rot_z(0, 1) = -std::sin(angle_z);
        rot_z(1, 0) = std::sin(angle_z);
        rot_z(1, 1) = std::cos(angle_z);
        m = rot_z * m;
    }

    return m;
}

mat4f inverse(const mat4f& m) noexcept
{
    // taken from GLM source code.
    float Coef00 = m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3);
    float Coef02 = m(1, 2) * m(3, 3) - m(3, 2) * m(1, 3);
    float Coef03 = m(1, 2) * m(2, 3) - m(2, 2) * m(1, 3);

    float Coef04 = m(2, 1) * m(3, 3) - m(3, 1) * m(2, 3);
    float Coef06 = m(1, 1) * m(3, 3) - m(3, 1) * m(1, 3);
    float Coef07 = m(1, 1) * m(2, 3) - m(2, 1) * m(1, 3);

    float Coef08 = m(2, 1) * m(3, 2) - m(3, 1) * m(2, 2);
    float Coef10 = m(1, 1) * m(3, 2) - m(3, 1) * m(1, 2);
    float Coef11 = m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2);

    float Coef12 = m(2, 0) * m(3, 3) - m(3, 0) * m(2, 3);
    float Coef14 = m(1, 0) * m(3, 3) - m(3, 0) * m(1, 3);
    float Coef15 = m(1, 0) * m(2, 3) - m(2, 0) * m(1, 3);

    float Coef16 = m(2, 0) * m(3, 2) - m(3, 0) * m(2, 2);
    float Coef18 = m(1, 0) * m(3, 2) - m(3, 0) * m(1, 2);
    float Coef19 = m(1, 0) * m(2, 2) - m(2, 0) * m(1, 2);

    float Coef20 = m(2, 0) * m(3, 1) - m(3, 0) * m(2, 1);
    float Coef22 = m(1, 0) * m(3, 1) - m(3, 0) * m(1, 1);
    float Coef23 = m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1);

    vec4f Fac0{Coef00, Coef00, Coef02, Coef03};
    vec4f Fac1{Coef04, Coef04, Coef06, Coef07};
    vec4f Fac2{Coef08, Coef08, Coef10, Coef11};
    vec4f Fac3{Coef12, Coef12, Coef14, Coef15};
    vec4f Fac4{Coef16, Coef16, Coef18, Coef19};
    vec4f Fac5{Coef20, Coef20, Coef22, Coef23};

    vec4f Vec0{m(1, 0), m(0, 0), m(0, 0), m(0, 0)};
    vec4f Vec1{m(1, 1), m(0, 1), m(0, 1), m(0, 1)};
    vec4f Vec2{m(1, 2), m(0, 2), m(0, 2), m(0, 2)};
    vec4f Vec3{m(1, 3), m(0, 3), m(0, 3), m(0, 3)};

    vec4f Inv0{vec4f{Vec1.x * Fac0.x, Vec1.y * Fac0.y, Vec1.z * Fac0.z, Vec1.w * Fac0.w} - vec4f{Vec2.x * Fac1.x, Vec2.y * Fac1.y, Vec2.z * Fac1.z, Vec2.w * Fac1.w} + vec4f{Vec3.x * Fac2.x, Vec3.y * Fac2.y, Vec3.z * Fac2.z, Vec3.w * Fac2.w}};
    vec4f Inv1{vec4f{Vec0.x * Fac0.x, Vec0.y * Fac0.y, Vec0.z * Fac0.z, Vec0.w * Fac0.w} - vec4f{Vec2.x * Fac3.x, Vec2.y * Fac3.y, Vec2.z * Fac3.z, Vec2.w * Fac3.w} + vec4f{Vec3.x * Fac4.x, Vec3.y * Fac4.y, Vec3.z * Fac4.z, Vec3.w * Fac4.w}};
    vec4f Inv2{vec4f{Vec0.x * Fac1.x, Vec0.y * Fac1.y, Vec0.z * Fac1.z, Vec0.w * Fac1.w} - vec4f{Vec1.x * Fac3.x, Vec1.y * Fac3.y, Vec1.z * Fac3.z, Vec1.w * Fac3.w} + vec4f{Vec3.x * Fac5.x, Vec3.y * Fac5.y, Vec3.z * Fac5.z, Vec3.w * Fac5.w}};
    vec4f Inv3{vec4f{Vec0.x * Fac2.x, Vec0.y * Fac2.y, Vec0.z * Fac2.z, Vec0.w * Fac2.w} - vec4f{Vec1.x * Fac4.x, Vec1.y * Fac4.y, Vec1.z * Fac4.z, Vec1.w * Fac4.w} + vec4f{Vec2.x * Fac5.x, Vec2.y * Fac5.y, Vec2.z * Fac5.z, Vec2.w * Fac5.w}};

    vec4f SignA{+1, -1, +1, -1};
    vec4f SignB{-1, +1, -1, +1};

    mat4f Inverse {};

    const auto A0 {vec4f{Inv0.x * SignA.x, Inv0.y * SignA.y, Inv0.z * SignA.z, Inv0.w * SignA.w}};
    const auto A1 {vec4f{Inv1.x * SignB.x, Inv1.y * SignB.y, Inv1.z * SignB.z, Inv1.w * SignB.w}};
    const auto A2 {vec4f{Inv2.x * SignA.x, Inv2.y * SignA.y, Inv2.z * SignA.z, Inv2.w * SignA.w}};
    const auto A3 {vec4f{Inv3.x * SignB.x, Inv3.y * SignB.y, Inv3.z * SignB.z, Inv3.w * SignB.w}};

    Inverse(0, 0) = A0.x;
    Inverse(0, 1) = A0.y;
    Inverse(0, 2) = A0.z;
    Inverse(0, 3) = A0.w;

    Inverse(1, 0) = A1.x;
    Inverse(1, 1) = A1.y;
    Inverse(1, 2) = A1.z;
    Inverse(1, 3) = A1.w;

    Inverse(2, 0) = A2.x;
    Inverse(2, 1) = A2.y;
    Inverse(2, 2) = A2.z;
    Inverse(2, 3) = A2.w;

    Inverse(3, 0) = A3.x;
    Inverse(3, 1) = A3.y;
    Inverse(3, 2) = A3.z;
    Inverse(3, 3) = A3.w;

    vec4f Row0(Inverse(0, 0), Inverse(1, 0), Inverse(2, 0), Inverse(3, 0));
    vec4f Dot0{m(0, 0)*Row0.x, m(0, 1)*Row0.y, m(0, 2)*Row0.z, m(0, 3)*Row0.w};
    float Dot1 = (Dot0.x + Dot0.y) + (Dot0.z + Dot0.w);

    float OneOverDeterminant = static_cast<float>(1) / Dot1;

    for (std::size_t i{}; i<4; ++i) {
        for (std::size_t j{}; j<4; ++j) {
            Inverse(i, j) *= OneOverDeterminant;
        }
    }
    return Inverse;
}

}
