#pragma once

#include <SDL3/SDL_video.h>

#include <vector>

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
    void draw();
    void test_draw();

    // gl entities.
    gl::shader circle_shader;
    gl::shader textured_quad_shader;
    gl::vertex_array vao;
    gl::named_buffer vbo;
    gl::named_buffer ibo;

    gl::vertex_array canvas_vao;
    gl::named_buffer canvas_vbo;

    math::mat4f window_projection;

    struct canvas {
        gl::texture2d texture;
        gl::sampler sampler;
        gl::frame_buffer buffer;

        int width  {};
        int height {};
        float pos_x {};
        float pos_y {};
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
        float world_offset_x {};
        float world_offset_y {};
        bool mouse_moved {};
        float pan_speed {50.0f};
        float resize_speed {100.0f};
        float brush_size {10.0f};
        bool should_draw {false};
        bool should_empty {false};
        bool init {true};
        bool resized {true};
    } info;

    std::vector<float> cpx;
    std::vector<float> cpy;
    std::vector<float> cpr;

    struct line {
        float x1 {}, y1 {};
        float x2 {}, y2 {};
        float thickness_1 {};
        float thickness_2 {};
    };
    std::vector<line> lines;

    /*
    struct test_data {
        struct line {
            float x1 {}, y1 {};
            float x2 {}, y2 {};
        };

        struct quad {
            float x1 {}, y1 {};
            float x2 {}, y2 {};
            float x3 {}, y3 {};
            float x4 {}, y4 {};
            float thickness {5.0f};
            float len {};
        };

        bool should_do {false};
        std::vector<line> lines;
        std::vector<quad> quads; // treat as quad
        line ll {};
    } test_data;
    */

    gl::vertex_array line_vao;
    gl::named_buffer line_vbo;
    gl::named_buffer line_ibo;
    gl::shader line_shader;

    gl::vertex_array line_v2_vao;
    gl::named_buffer line_v2_vbo;
    gl::shader line_v2_shader;

};

}
