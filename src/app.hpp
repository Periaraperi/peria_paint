#pragma once

#include <SDL3/SDL_video.h>

#include "graphics/gl_entities.hpp"
#include "graphics/shader.hpp"

namespace peria {

struct application_settings {
    const char* title {"application"};
    int window_width  {800};
    int window_height {600};
    bool resizable {false};
};

namespace sdl {

struct sdl_initializer {
    sdl_initializer(const application_settings& settings) noexcept;

    sdl_initializer(const sdl_initializer&) = delete;
    sdl_initializer& operator=(const sdl_initializer&) = delete;

    sdl_initializer(sdl_initializer&&) noexcept = default;
    sdl_initializer& operator=(sdl_initializer&&) noexcept = default;

    ~sdl_initializer();

    bool initialized {true};

    SDL_Window* window {nullptr};
    SDL_GLContext context {nullptr};
};

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
    application_settings app_settings_;
    sdl::sdl_initializer sdl_initializer_;

    void update();
    void draw();

    // gl entities.
    gl::shader shader;
    gl::vertex_array vao;
    gl::named_buffer vbo;
    gl::named_buffer ibo;

};

}
