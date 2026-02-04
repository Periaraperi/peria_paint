#pragma once

#include <array>
#include <cstddef>
#include <sstream>
#include <string>
#include "math/vec.hpp"

namespace peria::math {

template<typename T, std::size_t N, std::size_t M>
class matrix {
public:
    matrix() noexcept = default;
    explicit matrix(T x) noexcept
    {
        for (std::size_t r{}; r<N; ++r) {
            for (std::size_t c{}; c<M; ++c) {
                data_[r*M + c] = x;
            }
        }
    }
    explicit matrix(const std::array<std::array<T, M>, N>& data) noexcept
    {
        for (std::size_t r{}; r<N; ++r) {
            for (std::size_t c{}; c<M; ++c) {
                data_[r*M + c] = data[r][c];
            }
        }
    }

    [[nodiscard]]
    float& operator()(std::size_t r, std::size_t c) noexcept
    { return data_[r*M + c]; }

    [[nodiscard]]
    const float& operator()(std::size_t r, std::size_t c) const noexcept
    { return data_[r*M + c]; }

    template <std::size_t U, std::size_t V>
    [[nodiscard]] matrix<T, N, V> operator*(const matrix<T, U, V>& rhs) const noexcept
    {
        static_assert(M == U, "Columns of LHS should be equal to rows of RHS");
        matrix<T, N, V> res{T{}};
        for (std::size_t r{}; r<N; ++r) {
            for (std::size_t c{}; c<V; ++c) {
                for (std::size_t k{}; k<U; ++k) {
                    res(r, c) += this->operator()(r, k) * rhs(k, c);
                }
            }
        }
        return res;
    }

    [[nodiscard]] vec4<T> operator*(const vec4<T>& v) const noexcept
    {
        static_assert(M == 4, "Expected matrix with 4 columns and vector of 4 elements");
        vec4<T> res {};
        res.x = this->operator()(0, 0)*v.x+this->operator()(0, 1)*v.y+this->operator()(0, 2)*v.z+this->operator()(0, 3)*v.w;
        res.y = this->operator()(1, 0)*v.x+this->operator()(1, 1)*v.y+this->operator()(1, 2)*v.z+this->operator()(1, 3)*v.w;
        res.z = this->operator()(2, 0)*v.x+this->operator()(2, 1)*v.y+this->operator()(2, 2)*v.z+this->operator()(2, 3)*v.w;
        res.w = this->operator()(3, 0)*v.x+this->operator()(3, 1)*v.y+this->operator()(3, 2)*v.z+this->operator()(3, 3)*v.w;
        return res;
    }

    [[nodiscard]]
    const float* data() const noexcept
    { return data_.data(); }

    [[nodiscard]]
    matrix<T, N, M> transpose() const noexcept
    {
        static_assert(N == M, "transpose only works for square matrices");
        matrix<T, N, M> m {};
        for (std::size_t r{}; r<N; ++r) {
            for (std::size_t c{}; c<M; ++c) {
                m(r, c) = this->operator()(c, r);
            }
        }
        return m;
    }

    [[nodiscard]]
    std::string to_string() const
    {
        std::stringstream ss;
        for (std::size_t r{}; r<N; ++r) {
            for (std::size_t c{}; c<M; ++c) {
                ss << (std::to_string(this->operator()(r, c)) + " ");
            }
            ss << '\n';
        }
        return ss.str();
    }

private:
    std::array<T, N*M> data_ {};
};

using mat4f = matrix<float, 4, 4>;

static const mat4f MAT4F_IDENTITY {{
    {{1.0f, 0.0f, 0.0f, 0.0f},
     {0.0f, 1.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 1.0f, 0.0f},
     {0.0f, 0.0f, 0.0f, 1.0f}},
}};

[[nodiscard]]
mat4f get_ortho_projection(float left, float right, float bottom, float top) noexcept;

[[nodiscard]]
mat4f translate(float x, float y, float z) noexcept;

[[nodiscard]]
mat4f scale(float x, float y, float z) noexcept;

[[nodiscard]]
mat4f rotate(float x, float y, float z) noexcept;

[[nodiscard]]
mat4f inverse(const mat4f& m) noexcept;

}
