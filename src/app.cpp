#include "app.hpp"

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <print>
#include <utility>

namespace peria {

namespace sdl {

sdl_initializer::sdl_initializer() noexcept
{
    initialized = SDL_Init(SDL_INIT_VIDEO);
    if (!initialized) {
        std::print("{}\n", SDL_GetError());
        return;
    }

    // TODO: add checks later
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
};

sdl_initializer::~sdl_initializer()
{
    std::print("Shutting down SDL3\n");
    SDL_Quit();
}

void sdl_window_deleter::operator()(SDL_Window* window) const noexcept
{ std::print("Destroying SDL_Window\n"); SDL_DestroyWindow(window); }

void gl_context_deleter::operator()(SDL_GLContext context) const noexcept
{ std::print("Destroying SDL_GLContext\n"); SDL_GL_DestroyContext(context); }

} // end sdl

application::application(application_settings&& settings)
    :app_settings_{std::move(settings)}
{
    std::print("application construction\n");
    
    auto window_flags {SDL_WINDOW_OPENGL};
    if (app_settings_.resizable) window_flags |= SDL_WINDOW_RESIZABLE;
    window_ = std::unique_ptr<SDL_Window, sdl::sdl_window_deleter>(
        SDL_CreateWindow(app_settings_.title, 
                         app_settings_.window_width, 
                         app_settings_.window_height, 
                         window_flags)
    );
    if (window_ == nullptr) {
        std::print("SDL Window failed. {}\n", SDL_GetError());
        return;
    }
    context_ = std::unique_ptr<SDL_GLContextState, sdl::gl_context_deleter>(
        SDL_GL_CreateContext(window_.get())
    );
    if (context_ == nullptr) {
        std::print("SDL GL Context failed. {}\n", SDL_GetError());
        return;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::print("GL loader failed\n");
        return;
    }

    input_manager::initialize();
}

application::~application()
{
    input_manager::shutdown();

    std::print("Shutting down application\n");
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
            }
        }

        if (!SDL_GL_SwapWindow(window_.get())) {
            std::print("SDL_GL_SwapWindow failed\n");
            running = false;
            break;
        }

        input_manager_->update_prev_state();

        SDL_Delay(1); // artifical delay of 1ms to not go bonkers
    }
}

}
