#include "graphics.hpp"

#include <SDL3/SDL.h>

namespace peria::graphics {

struct graphics_info {
    color bg {BLACK};
    int   screen_width    {800};
    int   screen_height   {600};
    bool  relative_mouse  {true};
    float mouse_x_rel     {}; // last rel movement
    float mouse_y_rel     {}; // last rel movement
} graphics_info;

void set_relative_motion(float x, float y) noexcept
{
    graphics_info.mouse_x_rel = x;
    graphics_info.mouse_y_rel = y;
}

void set_relative_mouse(SDL_Window* window, bool rel_mouse) noexcept
{
    graphics_info.relative_mouse = rel_mouse;
    if (graphics_info.relative_mouse) {
        SDL_SetWindowRelativeMouseMode(window, true);
    }
    else {
        SDL_SetWindowRelativeMouseMode(window, false);
    }
}

relative_motion get_relative_motion() noexcept
{ return {graphics_info.mouse_x_rel, graphics_info.mouse_y_rel}; }
  
void set_viewport(int x, int y, int w, int h) noexcept
{ glViewport(x, y, w, h); }

void set_vsync(bool vsync) noexcept
{ (vsync) ? SDL_GL_SetSwapInterval(1) : SDL_GL_SetSwapInterval(0); }

void set_screen_size(int w, int h) noexcept
{
    graphics_info.screen_width = w;
    graphics_info.screen_height = h;
}

screen_size get_screen_size() noexcept
{ return {graphics_info.screen_width, graphics_info.screen_height}; }

bool is_relative_mouse() noexcept
{ return graphics_info.relative_mouse; }

void bind_frame_buffer_default() noexcept
{ glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void bind_frame_buffer(const gl::frame_buffer& fbo) noexcept
{ glBindFramebuffer(GL_FRAMEBUFFER, fbo.id); }

void bind_vertex_array(const gl::vertex_array& vao) noexcept
{ glBindVertexArray(vao.id); }

void bind_texture_and_sampler(const gl::texture2d& texture, 
                              const gl::sampler& sampler, 
                              u32 unit) noexcept
{
    glBindTextureUnit(unit, texture.id);
    glBindSampler(unit, sampler.id);
}

void vao_connect_ibo(const gl::vertex_array& vao, const gl::named_buffer& ibo) noexcept
{ glVertexArrayElementBuffer(vao.id, ibo.id); }

void buffer_allocate_data(const gl::named_buffer& buffer, std::size_t bytes, u32 usage) noexcept
{ glNamedBufferData(buffer.id, static_cast<GLsizeiptr>(bytes), nullptr, usage); }

void buffer_upload_subdata(const gl::named_buffer& buffer, std::size_t buffer_offset, std::size_t data_size, const void* data) noexcept
{ glNamedBufferSubData(buffer.id, static_cast<GLintptr>(buffer_offset), static_cast<GLsizeiptr>(data_size), data); }

void clear_buffer_all(u32 fbo,
                      const color& color,
                      float depth_value,
                      int stencil_value) noexcept
{
    const auto& [r, g, b, a] = color;
    std::array<float, 4> cl {r, g, b, a};
    glClearNamedFramebufferfv(fbo, GL_COLOR, 0, cl.data());
    glClearNamedFramebufferfi(fbo, GL_DEPTH_STENCIL, 0, depth_value, stencil_value);
}

void clear_buffer_color(u32 fbo, const color& color) noexcept
{ 
    const auto& [r, g, b, a] = color;
    std::array<float, 4> cl {r, g, b, a};
    glClearNamedFramebufferfv(fbo, GL_COLOR, 0, cl.data()); 
}

void clear_buffer_depth(u32 fbo, float depth_value) noexcept
{ glClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &depth_value); }

gl::texture2d create_texture2d(int w, int h, u32 internal_format) noexcept
{
    gl::texture2d texture;
    glTextureStorage2D(texture.id, 1, internal_format, w, h);
    return texture;
}
gl::sampler create_sampler(int min_filter, int mag_filter, int wrap_s, int wrap_t, int wrap_r, const color& border_color) noexcept
{
    gl::sampler sampler;
    glSamplerParameteri(sampler.id, GL_TEXTURE_MIN_FILTER, min_filter);
    glSamplerParameteri(sampler.id, GL_TEXTURE_MAG_FILTER, mag_filter);
    glSamplerParameteri(sampler.id, GL_TEXTURE_WRAP_S, wrap_s);
    glSamplerParameteri(sampler.id, GL_TEXTURE_WRAP_T, wrap_t);
    glSamplerParameteri(sampler.id, GL_TEXTURE_WRAP_R, wrap_r);
    if (wrap_s == GL_CLAMP_TO_BORDER || wrap_t == GL_CLAMP_TO_BORDER || wrap_r == GL_CLAMP_TO_BORDER) {
        const auto& [r, g, b, _] {border_color};
        const std::array clr {r, g, b};
        glSamplerParameterfv(sampler.id, GL_TEXTURE_BORDER_COLOR, clr.data());
    }
    return sampler;
}


}
