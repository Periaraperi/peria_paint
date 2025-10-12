#include "app.hpp"

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <print>
#include <utility>

#include "input_manager.hpp"
#include "graphics/graphics.hpp"

namespace peria {

namespace sdl {

sdl_initializer::sdl_initializer(const application_settings& settings) noexcept
{
    std::println("Initializing SDL");
    initialized = SDL_Init(SDL_INIT_VIDEO);
    if (!initialized) {
        std::println("{}\n", SDL_GetError());
        return;
    }

    // TODO: add checks later
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    auto window_flags {SDL_WINDOW_OPENGL};
    if (settings.resizable) window_flags |= SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow(settings.title, 
                         settings.window_width, 
                         settings.window_height, 
                         window_flags);
    if (window == nullptr) {
        initialized = false;
        std::println("SDL Window failed. {}", SDL_GetError());
        return;
    }
    context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        initialized = false;
        std::println("SDL GL Context failed. {}", SDL_GetError());
        return;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        initialized = false;
        std::println("GL loader failed");
        return;
    }

    std::println("Successfully initialized SDL, SDL_Window, SDL_GLContext, and GLAD");
};

sdl_initializer::~sdl_initializer()
{
    std::println("Shutting down SDL3");
    SDL_DestroyWindow(window);
    SDL_GL_DestroyContext(context);
    SDL_Quit();
}

} // end sdl

application::application(application_settings&& settings)
    :app_settings_{std::move(settings)},
     sdl_initializer_{app_settings_},
     shader{"./assets/shaders/default.vert", "./assets/shaders/default.frag"},
     canvas{gl::shader{"./assets/shaders/canvas.vert", "./assets/shaders/canvas.frag"},
            gl::texture2d{graphics::create_texture2d(400, 300, GL_RGBA8)},
            gl::sampler{graphics::create_sampler(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER)}, {}, 400, 300, {}}
{
    if (!sdl_initializer_.initialized) return;
    std::println("application construction");

    input_manager::initialize();

    {
        //TODO: come back to these settings later
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glEnable(GL_DEPTH_TEST);
        //glEnable(GL_STENCIL_TEST);
        //glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        peria::graphics::set_vsync(true);
        peria::graphics::set_relative_mouse(sdl_initializer_.window, false);
        peria::graphics::set_screen_dimensions(app_settings_.window_width, app_settings_.window_height);
        window_proj = math::get_ortho_projection(0.0f, static_cast<float>(app_settings_.window_width), 0.0f, static_cast<float>(app_settings_.window_height));
    }
    // GL entities here
    {
        std::array<gl::vertex<gl::pos2>, 4> data {{
            {{-0.5f, -0.5f}},
            {{-0.5f,  0.5f}},
            {{ 0.5f,  0.5f}},
            {{ 0.5f, -0.5f}},
        }};
        std::array<u32, 6> indices {0,1,2, 0,2,3};

        graphics::buffer_upload_data(vbo, data, GL_STATIC_DRAW);
        graphics::buffer_upload_data(ibo, indices, GL_STATIC_DRAW);

        graphics::vao_configure<gl::pos2>(vao, vbo, 0);
        graphics::vao_connect_ibo(vao, ibo);

        std::array<gl::vertex<gl::pos2, gl::texture_coord>, 4> canvas_data {{
            {{-0.5f, -0.5f}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f}, {0.0f, 1.0f}},
            {{ 0.5f,  0.5f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f}, {1.0f, 0.0f}},
        }};

        graphics::buffer_upload_data(canvas_vbo, canvas_data, GL_STATIC_DRAW);

        graphics::vao_configure<gl::pos2, gl::texture_coord>(canvas_vao, canvas_vbo, 0);
        graphics::vao_connect_ibo(canvas_vao, ibo);

        glNamedFramebufferTexture(canvas.buffer.id, GL_COLOR_ATTACHMENT0, canvas.texture.id, 0);
        const auto status {glCheckNamedFramebufferStatus(canvas.buffer.id, GL_FRAMEBUFFER)};
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::println("FrameBuffer with id {} is incomplete\n {}", canvas.buffer.id, status);
        }

        canvas.shader.set_int("u_canvas_texture", 0);
        canvas.proj = math::get_ortho_projection(0.0f, static_cast<float>(canvas.tex_w), 0.0f, static_cast<float>(canvas.tex_h));
    }

}

application::~application()
{
    input_manager::shutdown();

    std::println("Shutting down application");
}

bool application::initialized() const noexcept
{ return sdl_initializer_.initialized; }

static bool done {false};

void application::run()
{
    bool running {true};

    auto input_manager_ {input_manager::instance()};

    while (running) {
        input_manager_->update_mouse();

        // Poll for events, and react to window resize and mouse movement events here
        for (SDL_Event ev; SDL_PollEvent(&ev);) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }
            else if (ev.type == SDL_EVENT_WINDOW_RESIZED) {
                app_settings_.window_width = ev.window.data1;
                app_settings_.window_height = ev.window.data2;
                graphics::set_viewport(0, 0, app_settings_.window_width, app_settings_.window_height);
                graphics::set_screen_dimensions(app_settings_.window_width, app_settings_.window_height);
                window_proj = math::get_ortho_projection(0.0f, static_cast<float>(app_settings_.window_width), 0.0f, static_cast<float>(app_settings_.window_height));
            }
            else if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                peria::graphics::set_relative_motion(ev.motion.xrel, -ev.motion.yrel);
            }
        }

        update();

        draw();

        if (!SDL_GL_SwapWindow(sdl_initializer_.window)) {
            std::println("SDL_GL_SwapWindow failed");
            running = false;
            break;
        }

        input_manager_->update_prev_state();

        SDL_Delay(1); // artificial delay of 1ms to not go bonkers
    }
}

void application::update()
{
}

void application::draw()
{
    const auto w {static_cast<float>(app_settings_.window_width)};
    const auto h {static_cast<float>(app_settings_.window_height)};

    const auto cw {static_cast<float>(canvas.tex_w)};
    const auto ch {static_cast<float>(canvas.tex_h)};

    if (!done) {
        std::println("DRAWING TO OFFSCREEN BUFFER ONCE, AND REUSING IT LATER");
        graphics::bind_frame_buffer(canvas.buffer);
        graphics::set_viewport(0, 0, canvas.tex_w, canvas.tex_h);
        graphics::clear_buffer_color(canvas.buffer.id, graphics::AQUA);
        graphics::bind_vertex_array(vao);

        math::mat4f model {math::translate(cw*0.5f, ch*0.5f, 0.0f)*
                           math::scale(cw*0.25f, ch*0.25f, 1.0f)};
        shader.use_shader();
        shader.set_mat4("u_mvp", canvas.proj*model);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        done = true;
    }

    graphics::bind_frame_buffer_default();
    graphics::clear_buffer_all(0, graphics::INDIGO, 1.0f, 0);
    graphics::set_viewport(0, 0, app_settings_.window_width, app_settings_.window_height);

    math::mat4f model {math::translate(w*0.5f, h*0.5f, 0.0f)*
                       math::scale(cw, ch, 1.0f)};

    graphics::bind_vertex_array(canvas_vao);
    canvas.shader.use_shader();
    canvas.shader.set_mat4("u_mvp", window_proj*model);
    graphics::bind_texture_and_sampler(canvas.texture, canvas.sampler, 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

}
