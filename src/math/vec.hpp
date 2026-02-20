#pragma once

#include <cmath>
#include <type_traits>

namespace peria::math {

template<typename T>
struct vec2;
template<typename T>
struct vec3;
template<typename T>
struct vec4;

template<typename T>
[[nodiscard]] constexpr T dot(const vec2<T>& lhs, const vec2<T>& rhs) noexcept
{ return lhs.x*rhs.x + lhs.y*rhs.y; }

template<typename T>
[[nodiscard]] constexpr T dot(const vec3<T>& lhs, const vec3<T>& rhs) noexcept
{ return lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z; }

template<typename T>
[[nodiscard]] constexpr T dot(const vec4<T>& lhs, const vec4<T>& rhs) noexcept
{ return lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z + lhs.w+rhs.w; }

template<typename T>
struct vec2 {
    static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>, "T must be integral or floating point type");
    constexpr vec2() noexcept = default;
    constexpr explicit vec2(const T value) noexcept
        :x{value}, y{value}
    {}
    constexpr vec2(const T x_, const T y_) noexcept
        :x{x_}, y{y_}
    {}
    
    [[nodiscard]] constexpr vec2<T> operator+(const vec2<T>& rhs) noexcept
    { return {x+rhs.x, y+rhs.y}; }

    [[nodiscard]] constexpr vec2<T> operator-(const vec2<T>& rhs) noexcept
    { return {x-rhs.x, y-rhs.y}; }

    [[nodiscard]] constexpr vec2<T> operator+(const T s) noexcept
    { return {x+s, y+s}; }

    [[nodiscard]] constexpr vec2<T> operator-(const T s) noexcept
    { return {x-s, y-s}; }
    
    [[nodiscard]] constexpr vec2<T> operator*(const T s) noexcept
    { return {x*s, y*s}; }

    constexpr vec2<T>& operator+=(const vec2<T>& rhs) noexcept
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr vec2<T>& operator+=(const T s) noexcept
    {
        x += s;
        y += s;
        return *this;
    }

    constexpr vec2<T>& operator-=(const vec2<T>& rhs) noexcept
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr vec2<T>& operator-=(const T s) noexcept
    {
        x -= s;
        y -= s;
        return *this;
    }

    constexpr vec2<T>& operator*=(const T s) noexcept
    {
        x *= s;
        y *= s;
        return *this;
    }

    [[nodiscard]] friend constexpr vec2<T> operator+(const vec2<T>& lhs, const vec2<T>& rhs) noexcept
    { return {lhs.x+rhs.x, lhs.y+rhs.y}; }

    [[nodiscard]] friend constexpr vec2<T> operator+(const T s, const vec2<T>& rhs) noexcept
    { return {s+rhs.x, s+rhs.y}; }

    [[nodiscard]] friend constexpr vec2<T> operator+(const vec2<T>& lhs, const T s) noexcept
    { return {lhs.x+s, lhs.y+s}; }
    
    [[nodiscard]] friend constexpr vec2<T> operator-(const vec2<T>& lhs, const vec2<T>& rhs) noexcept
    { return {lhs.x-rhs.x, lhs.y-rhs.y}; }

    [[nodiscard]] friend constexpr vec2<T> operator-(const T s, const vec2<T>& rhs) noexcept
    { return {s-rhs.x, s-rhs.y}; }

    [[nodiscard]] friend constexpr vec2<T> operator-(const vec2<T>& lhs, const T s) noexcept
    { return {lhs.x-s, lhs.y-s}; }

    [[nodiscard]] friend constexpr vec2<T> operator*(const T s, const vec2<T>& rhs) noexcept
    { return {s*rhs.x, s*rhs.y}; }

    [[nodiscard]] friend constexpr vec2<T> operator*(const vec2<T>& lhs, const T s) noexcept
    { return {lhs.x*s, lhs.y*s}; }

    [[nodiscard]] constexpr T len() noexcept
    { return std::sqrt(x*x + y*y); }

    [[nodiscard]] constexpr vec2<T> normalize() noexcept
    {
        const auto l {len()};
        return {x/l, y/l};
    }

    [[nodiscard]] constexpr T dot(const vec2<T>& rhs) noexcept
    { return x*rhs.x + y*rhs.y; }

    T x {};
    T y {};
};

template<typename T>
struct vec3 {
    static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>, "T must be integral or floating point type");
    constexpr vec3() noexcept = default;
    constexpr explicit vec3(const T value) noexcept
        :x{value}, y{value}, z{value}
    {}
    constexpr vec3(const T x_, const T y_, const T z_) noexcept
        :x{x_}, y{y_}, z{z_}
    {}
    constexpr vec3(const vec2<T>& v, const T z_) noexcept
        :x{v.x}, y{v.y}, z{z_}
    {}
    
    [[nodiscard]] constexpr vec3<T> operator+(const vec3<T>& rhs) noexcept
    { return {x+rhs.x, y+rhs.y, z+rhs.z}; }

    [[nodiscard]] constexpr vec3<T> operator-(const vec3<T>& rhs) noexcept
    { return {x-rhs.x, y-rhs.y, z-rhs.z}; }

    [[nodiscard]] constexpr vec3<T> operator+(const T s) noexcept
    { return {x+s, y+s, z+s}; }

    [[nodiscard]] constexpr vec3<T> operator-(const T s) noexcept
    { return {x-s, y-s, z-s}; }
    
    [[nodiscard]] constexpr vec3<T> operator*(const T s) noexcept
    { return {x*s, y*s, z*s}; }

    constexpr vec3<T>& operator+=(const vec3<T>& rhs) noexcept
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr vec3<T>& operator+=(const T s) noexcept
    {
        x += s;
        y += s;
        z += s;
        return *this;
    }

    constexpr vec3<T>& operator-=(const vec3<T>& rhs) noexcept
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr vec3<T>& operator-=(const T s) noexcept
    {
        x -= s;
        y -= s;
        z -= s;
        return *this;
    }

    constexpr vec3<T>& operator*=(const T s) noexcept
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    [[nodiscard]] friend constexpr vec3<T> operator+(const vec3<T>& lhs, const vec3<T>& rhs) noexcept
    { return {lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z}; }

    [[nodiscard]] friend constexpr vec3<T> operator+(const T s, const vec3<T>& rhs) noexcept
    { return {s+rhs.x, s+rhs.y, s+rhs.z}; }

    [[nodiscard]] friend constexpr vec3<T> operator+(const vec3<T>& lhs, const T s) noexcept
    { return {lhs.x+s, lhs.y+s, lhs.z+s}; }
    
    [[nodiscard]] friend constexpr vec3<T> operator-(const vec3<T>& lhs, const vec3<T>& rhs) noexcept
    { return {lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z}; }

    [[nodiscard]] friend constexpr vec3<T> operator-(const T s, const vec3<T>& rhs) noexcept
    { return {s-rhs.x, s-rhs.y, s-rhs.z}; }

    [[nodiscard]] friend constexpr vec3<T> operator-(const vec3<T>& lhs, const T s) noexcept
    { return {lhs.x-s, lhs.y-s, lhs.z-s}; }

    [[nodiscard]] friend constexpr vec3<T> operator*(const T s, const vec3<T>& rhs) noexcept
    { return {s*rhs.x, s*rhs.y, s*rhs.z}; }

    [[nodiscard]] friend constexpr vec3<T> operator*(const vec3<T>& lhs, const T s) noexcept
    { return {lhs.x*s, lhs.y*s, lhs.z*s}; }

    [[nodiscard]] constexpr T len() noexcept
    { return std::sqrt(x*x + y*y + z*z); }

    [[nodiscard]] constexpr vec3<T> normalize() noexcept
    {
        const auto l {len()};
        return {x/l, y/l, z/l};
    }

    [[nodiscard]] constexpr T dot(const vec3<T>& rhs) noexcept
    { return x*rhs.x + y*rhs.y + z*rhs.z; }

    [[nodiscard]] constexpr vec2<T> xy() noexcept
    { return {x, y}; }

    T x {};
    T y {};
    T z {};
};

template<typename T>
struct vec4 {
    static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>, "T must be integral or floating point type");
    constexpr vec4() noexcept = default;
    constexpr explicit vec4(const T value) noexcept
        :x{value}, y{value}, z{value}, w{value}
    {}
    constexpr vec4(const T x_, const T y_, const T z_, const T w_) noexcept
        :x{x_}, y{y_}, z{z_}, w{w_}
    {}

    constexpr vec4(const vec2<T>& v, const T z_, const T w_) noexcept
        :x{v.x}, y{v.y}, z{z_}, w{w_}
    {}

    constexpr vec4(const vec3<T>& v, const T w_) noexcept
        :x{v.x}, y{v.y}, z{v.z}, w{w_}
    {}
    
    [[nodiscard]] constexpr vec4<T> operator+(const vec4<T>& rhs) noexcept
    { return {x+rhs.x, y+rhs.y, z+rhs.z, w+rhs.w}; }

    [[nodiscard]] constexpr vec4<T> operator-(const vec4<T>& rhs) noexcept
    { return {x-rhs.x, y-rhs.y, z-rhs.z, w-rhs.w}; }

    [[nodiscard]] constexpr vec4<T> operator+(const T s) noexcept
    { return {x+s, y+s, z+s, w+s}; }

    [[nodiscard]] constexpr vec4<T> operator-(const T s) noexcept
    { return {x-s, y-s, z-s, w-s}; }
    
    [[nodiscard]] constexpr vec4<T> operator*(const T s) noexcept
    { return {x*s, y*s, z*s, w*s}; }

    constexpr vec4<T>& operator+=(const vec4<T>& rhs) noexcept
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    constexpr vec4<T>& operator+=(const T s) noexcept
    {
        x += s;
        y += s;
        z += s;
        w += s;
        return *this;
    }

    constexpr vec4<T>& operator-=(const vec4<T>& rhs) noexcept
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    constexpr vec4<T>& operator-=(const T s) noexcept
    {
        x -= s;
        y -= s;
        z -= s;
        w -= s;
        return *this;
    }

    constexpr vec4<T>& operator*=(const T s) noexcept
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    [[nodiscard]] friend constexpr vec4<T> operator+(const vec4<T>& lhs, const vec4<T>& rhs) noexcept
    { return {lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z, lhs.w+rhs.w}; }

    [[nodiscard]] friend constexpr vec4<T> operator+(const T s, const vec4<T>& rhs) noexcept
    { return {s+rhs.x, s+rhs.y, s+rhs.z, s+rhs.w}; }

    [[nodiscard]] friend constexpr vec4<T> operator+(const vec4<T>& lhs, const T s) noexcept
    { return {lhs.x+s, lhs.y+s, lhs.z+s, lhs.w+s}; }
    
    [[nodiscard]] friend constexpr vec4<T> operator-(const vec4<T>& lhs, const vec4<T>& rhs) noexcept
    { return {lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z, lhs.w-rhs.w}; }

    [[nodiscard]] friend constexpr vec4<T> operator-(const T s, const vec4<T>& rhs) noexcept
    { return {s-rhs.x, s-rhs.y, s-rhs.z, s-rhs.w}; }

    [[nodiscard]] friend constexpr vec4<T> operator-(const vec4<T>& lhs, const T s) noexcept
    { return {lhs.x-s, lhs.y-s, lhs.z-s, lhs.w-s}; }

    [[nodiscard]] friend constexpr vec4<T> operator*(const T s, const vec4<T>& rhs) noexcept
    { return {s*rhs.x, s*rhs.y, s*rhs.z, s*rhs.w}; }

    [[nodiscard]] friend constexpr vec4<T> operator*(const vec4<T>& lhs, const T s) noexcept
    { return {lhs.x*s, lhs.y*s, lhs.z*s, lhs.w*s}; }

    [[nodiscard]] constexpr T len() noexcept
    { return std::sqrt(x*x + y*y + z*z + w*w); }

    [[nodiscard]] constexpr vec4<T> normalize() noexcept
    {
        const auto l {len()};
        return {x/l, y/l, z/l, w/l};
    }

    [[nodiscard]] constexpr T dot(const vec4<T>& rhs) noexcept
    { return x*rhs.x + y*rhs.y + z*rhs.z + w*rhs.w; }

    [[nodiscard]] constexpr vec2<T> xy() noexcept
    { return {x, y}; }

    [[nodiscard]] constexpr vec3<T> xyz() noexcept
    { return {x, y, z}; }

    T x {};
    T y {};
    T z {};
    T w {};
};

using vec2f = peria::math::vec2<float>;
using vec3f = peria::math::vec3<float>;
using vec4f = peria::math::vec4<float>;

using vec2i = peria::math::vec2<int>;
using vec3i = peria::math::vec3<int>;
using vec4i = peria::math::vec4<int>;

}
