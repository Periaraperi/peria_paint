#include "graphics.hpp"

#include <SDL3/SDL.h>

namespace peria::graphics {

struct graphics_info {
    color bg {BLACK};
    int   screen_width    {800};
    int   screen_height   {600};
    bool  relative_mouse  {true};
    float mouse_x_rel     {};
    float mouse_y_rel     {};
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
  
void set_viewport(int x, int y, int w, int h) noexcept
{ glViewport(x, y, w, h); }

void set_vsync(bool vsync) noexcept
{ (vsync) ? SDL_GL_SetSwapInterval(1) : SDL_GL_SetSwapInterval(0); }

void set_screen_dimensions(int w, int h) noexcept
{
    graphics_info.screen_width = w;
    graphics_info.screen_height = h;
}

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

}
