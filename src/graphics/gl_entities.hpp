#pragma once

#include <cstdint>

namespace peria::graphics {

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

}
