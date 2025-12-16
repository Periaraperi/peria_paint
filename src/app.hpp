#pragma once

#include <SDL3/SDL_video.h>

#include <vector>

#include "graphics/gl_entities.hpp"
#include "graphics/shader.hpp"

#include "math/vec.hpp"

namespace peria {

struct application_settings {
    const char* title {"application"};
    int window_width  {800};
    int window_height {600};
    bool resizable {false};
};

struct brush_point {
    vec2 p;
    float r {};
};

// #TODO: change this later 
struct line {
    vec2 p1, p2;
    float thickness_1 {};
    float thickness_2 {};
};

namespace sdl {

struct sdl_initializer {
    explicit sdl_initializer(const application_settings& settings) noexcept;

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

    void update(float dt);
    void test_update(float dt);
    void draw();
    void test_draw();

    // gl entities.
    gl::vertex_array circle_vao;
    gl::vertex_array canvas_vao;
    gl::vertex_array line_vao;
    gl::vertex_array line_resize_vao;

    gl::named_buffer circle_vbo;
    gl::named_buffer canvas_vbo;
    gl::named_buffer line_vbo;
    gl::named_buffer line_resize_vbo;

    gl::named_buffer quad_ibo;
    gl::named_buffer line_ibo;
    gl::named_buffer line_resize_ibo;

    gl::shader circle_shader;
    gl::shader textured_quad_shader;
    gl::shader line_shader;

    math::mat4f window_projection;

    struct canvas {
        gl::texture2d texture;
        gl::sampler sampler;
        gl::frame_buffer buffer;

        int width  {};
        int height {};
        vec2 pos {};
        math::mat4f projection;
        graphics::color bg_color;
    } canvas;

    struct temp_canvas {
        gl::texture2d texture;
        gl::frame_buffer buffer;
        int width  {};
        int height {};
    } temp_canvas;

    struct info {
        vec2 world_offset {};
        bool mouse_moved {};
        bool prev_mouse_moved {};
        float pan_speed {50.0f};
        float resize_speed {100.0f};
        float brush_size {10.0f};
        bool should_draw {false};
        bool should_empty {false};
        bool resized {true};
        bool resizing {false};
        int new_width {};
        int new_height {};
    } info;

    std::vector<brush_point> brush_points; // for brush stroke

    static constexpr int MAX_PER_BATCH {4096}; // count

    struct line_batcher {
        std::vector<gl::vertex<gl::pos2, gl::color4>> lines_data;
        using vertex_t = typename gl::vertex<gl::pos2, gl::color4>;
    } line_batcher;
};

}
