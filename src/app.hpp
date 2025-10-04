#pragma once

#include <SDL3/SDL_video.h>

#include <memory>

#include "input_manager.hpp"

namespace peria {

namespace sdl {

struct sdl_initializer {
    sdl_initializer() noexcept;

    sdl_initializer(const sdl_initializer&) = delete;
    sdl_initializer& operator=(const sdl_initializer&) = delete;

    sdl_initializer(sdl_initializer&&) noexcept = default;
    sdl_initializer& operator=(sdl_initializer&&) noexcept = default;

    ~sdl_initializer();

    bool initialized {};
};

struct sdl_window_deleter {
    void operator()(SDL_Window* window) const noexcept;
};

struct gl_context_deleter {
    void operator()(SDL_GLContext context) const noexcept;
};

};

struct application_settings {
    const char* title {"application"};
    int window_width  {800};
    int window_height {600};
    bool resizable {false};
};

class application {
public:
    explicit application(application_settings&& settings);

    application(const application&) = delete;
    application& operator=(const application&) = delete;
    application(application&&) = delete;
    application& operator=(application&&) = delete;
    ~application();

    [[nodiscard]]
    bool initialized() const noexcept;
    void run();

private:
    sdl::sdl_initializer sdl_initializer_;
    std::unique_ptr<SDL_Window, sdl::sdl_window_deleter> window_ {nullptr};
    std::unique_ptr<SDL_GLContextState, sdl::gl_context_deleter> context_ {nullptr};

    application_settings app_settings_;
};

}
