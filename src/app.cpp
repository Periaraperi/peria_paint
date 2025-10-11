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
     shader{"./assets/shaders/default.vert", "./assets/shaders/default.frag"}
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
        proj = math::get_ortho_projection(0.0f, static_cast<float>(app_settings_.window_width), 0.0f, static_cast<float>(app_settings_.window_height));
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
    }

}

application::~application()
{
    input_manager::shutdown();

    std::println("Shutting down application");
}

bool application::initialized() const noexcept
{ return sdl_initializer_.initialized; }

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
                proj = math::get_ortho_projection(0.0f, static_cast<float>(app_settings_.window_width), 0.0f, static_cast<float>(app_settings_.window_height));
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
    graphics::clear_buffer_all(0, graphics::INDIGO, 1.0f, 0);

    const auto w {static_cast<float>(app_settings_.window_width)};
    const auto h {static_cast<float>(app_settings_.window_height)};

    math::mat4f model {math::translate(w*0.5f, h*0.5f, 0.0f)*
                       math::scale(100.0f, 100.0f, 1.0f)};
    
    shader.use_shader();
    shader.set_mat4("u_mvp", proj*model);
    graphics::bind_vertex_array(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

}
