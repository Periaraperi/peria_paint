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

}
