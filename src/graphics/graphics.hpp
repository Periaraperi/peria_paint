#pragma once

#include <glad/glad.h>
#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "gl_entities.hpp"
#include "color.hpp"

using u32 = std::uint32_t;

namespace peria::graphics {

struct screen_size {
    int w {};
    int h {};
};

struct relative_motion {
    float x {};
    float y {};
};

// some screen/window stuff 
void set_relative_motion(float x, float y) noexcept;
void set_relative_mouse(SDL_Window* window, bool rel_mouse) noexcept;
void set_viewport(int x, int y, int w, int h) noexcept;
void set_vsync(bool vsync) noexcept;
bool is_vsync() noexcept;
void set_screen_size(int w, int h) noexcept;
[[nodiscard]] screen_size get_screen_size() noexcept;
[[nodiscard]] bool is_relative_mouse() noexcept;
[[nodiscard]] relative_motion get_relative_motion() noexcept;

// entity bindings
void bind_frame_buffer_default() noexcept;
void bind_frame_buffer(const gl::frame_buffer& fbo) noexcept;
void bind_vertex_array(const gl::vertex_array& vao) noexcept;
void bind_texture_and_sampler(const gl::texture2d& texture, 
                              const gl::sampler& sampler,
                              u32 unit = 0) noexcept;

// sets vertex attribute layout for specific VAO and binding index and connects VBO to it.
template <typename... Ts>
void vao_configure(const gl::vertex_array& vao, const gl::named_buffer& vbo, u32 binding_index) noexcept
{
    auto setup = [&](u32 attribute_index, int elem_count, std::size_t offset){
        glEnableVertexArrayAttrib(vao.id, attribute_index);
        glVertexArrayAttribBinding(vao.id, attribute_index, binding_index);
        glVertexArrayAttribFormat(vao.id, attribute_index, elem_count, GL_FLOAT, GL_FALSE, static_cast<GLuint>(offset));
    };

    [&]<std::size_t... I>(std::index_sequence<I...>) {
        std::size_t offset {0};
        (setup(I, Ts::elem_count, std::exchange(offset, offset+Ts::bytes)), ...);
    }(std::make_index_sequence<sizeof...(Ts)>{});

    glVertexArrayVertexBuffer(vao.id, binding_index, vbo.id, 0, gl::vertex<Ts...>::stride);
}

template <typename T, std::size_t N>
void buffer_upload_data(const gl::named_buffer& buffer, const std::array<T, N>& data, u32 usage) noexcept
{ 
    if constexpr (gl::is_vertex_v<T>) {
        glNamedBufferData(buffer.id, T::stride*data.size(), data.data(), usage); 
    }
    else {
        glNamedBufferData(buffer.id, sizeof(T)*data.size(), data.data(), usage); 
    }
}

template <typename T>
void buffer_upload_data(const gl::named_buffer& buffer, const std::vector<T>& data, u32 usage) noexcept
{ 
    if constexpr (gl::is_vertex_v<T>) {
        glNamedBufferData(buffer.id, T::stride*data.size(), data.data(), usage); 
    }
    else {
        glNamedBufferData(buffer.id, sizeof(T)*data.size(), data.data(), usage); 
    }
}

void vao_connect_ibo(const gl::vertex_array& vao, const gl::named_buffer& ibo) noexcept;

void buffer_allocate_data(const gl::named_buffer& buffer, std::size_t bytes, u32 usage) noexcept;

void buffer_upload_subdata(const gl::named_buffer& buffer, std::size_t buffer_offset, std::size_t data_size, const void* data) noexcept;

// clears frame buffer's color, depth, and stencil values.
void clear_buffer_all(u32 fbo,
                      const color& color,
                      float depth_value,
                      int stencil_value) noexcept;

// clears frame buffer's color.
void clear_buffer_color(u32 fbo, const color& color) noexcept;

// clears frame buffer's depth attachment.
void clear_buffer_depth(u32 fbo, float depth_value) noexcept;

gl::texture2d create_texture2d(int w, int h, u32 internal_format) noexcept;

gl::texture2d create_texture2d_from_image(const char* path, std::int32_t& w, std::int32_t& h, std::int32_t& channels, bool flip = true) noexcept;

gl::sampler create_sampler(int min_filter, int mag_filter, int wrap_s, int wrap_t, int wrap_r, const color& border_color=WHITE) noexcept;

std::string write_to_png(const gl::texture2d& texture, int width, int height, const char* filename = "") noexcept;

struct image_info {
    gl::texture2d texture {};
    std::int32_t width {};
    std::int32_t height {};
};
image_info load_png(const char* path) noexcept;

}
