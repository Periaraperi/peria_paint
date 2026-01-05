#pragma once

#include <cstdint>
#include <array>
#include <algorithm>
#include <type_traits>

namespace peria::gl {
struct vertex_array {
    vertex_array() noexcept;
    ~vertex_array();

    vertex_array(const vertex_array&) = delete;
    vertex_array& operator=(const vertex_array&) = delete;

    vertex_array(vertex_array&& rhs) noexcept;
    vertex_array& operator=(vertex_array&& rhs) noexcept;

    std::uint32_t id;
};

struct named_buffer {
    named_buffer() noexcept;
    ~named_buffer();

    named_buffer(const named_buffer&) = delete;
    named_buffer& operator=(const named_buffer&) = delete;

    named_buffer(named_buffer&& rhs) noexcept;
    named_buffer& operator=(named_buffer&& rhs) noexcept;

    std::uint32_t id;
};

struct texture2d {
    texture2d() noexcept;

    texture2d(const texture2d&) = delete;
    texture2d& operator=(const texture2d&) = delete;

    texture2d(texture2d&& rhs) noexcept;
    texture2d& operator=(texture2d&& rhs) noexcept;

    ~texture2d();

    std::uint32_t id;
};

struct sampler {
    sampler() noexcept;
    ~sampler();

    sampler(const sampler&) = delete;
    sampler& operator=(const sampler&) = delete;

    sampler(sampler&&) noexcept;
    sampler& operator=(sampler&&) noexcept;

    std::uint32_t id;
};

struct frame_buffer {
    frame_buffer() noexcept;
    ~frame_buffer();

    frame_buffer(const frame_buffer&) = delete;
    frame_buffer& operator=(const frame_buffer&) = delete;

    frame_buffer(frame_buffer&& rhs) noexcept;
    frame_buffer& operator=(frame_buffer&& rhs) noexcept;

    std::uint32_t id;
};

template <typename T, int D>
struct attr {
    static_assert(std::is_same_v<T, float> && D >= 1 && D <= 4, "Attribute must have type float and contain 1, 2, 3, or 4 elements");
    using type = T;
    static constexpr int elem_count {D};
    static constexpr std::size_t bytes {sizeof(T)*D};
    std::array<T, D> data;
};

template <typename... Ts>
struct vertex {
    static constexpr std::size_t stride {(Ts::bytes + ...)};
    std::array<std::common_type_t<typename Ts::type...>, (Ts::elem_count + ...)> arr;
    constexpr vertex() noexcept = default;
    constexpr vertex(const Ts&... attrs) 
    {
        std::size_t offset {};
        ((std::copy(attrs.data.begin(), attrs.data.end(), arr.begin()+offset), offset += Ts::elem_count), ...);
    }
};

template <typename T>
struct is_vertex : std::false_type {};

template <typename... Ts>
struct is_vertex<vertex<Ts...>> : std::true_type {};

template <typename T>
constexpr inline bool is_vertex_v = is_vertex<T>::value;

using pos2          = attr<float, 2>;
using pos3          = attr<float, 3>;
using pos4          = attr<float, 4>;
using normal        = attr<float, 3>;
using texture_coord = attr<float, 2>;
using color4        = attr<float, 4>;

}
