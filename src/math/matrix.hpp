#pragma once

#include <array>
#include <cstddef>
#include <sstream>
#include <string>

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
    [[nodiscard]] matrix<T, N, V> operator*(const matrix<T, U, V>& rhs) noexcept
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
    std::array<float, N*M> data_ {};
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

}
