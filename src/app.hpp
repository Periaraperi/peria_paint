#pragma once

#include <SDL3/SDL_video.h>

#include <vector>

#include "graphics/gl_entities.hpp"
#include "graphics/shader.hpp"

#include "math/camera2d.hpp"
#include "math/vec.hpp"

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

namespace imgui {

struct imgui {
    imgui() noexcept = default;
    imgui(SDL_Window* window, SDL_GLContext gl_context, const char* glsl_version) noexcept;

    imgui(const imgui&) = delete;
    imgui& operator=(const imgui&) = delete;

    imgui(imgui&&) noexcept = default;
    imgui& operator=(imgui&&) noexcept = default;

    ~imgui();

    [[nodiscard]]
    bool is_imgui_captured() noexcept;

    [[nodiscard]]
    bool is_imgui_hovered() noexcept;

    bool show_tools {true};
    bool pen_selected {true};
    bool eraser_selected {};
    bool bucket_selected {};
};

}

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
    imgui::imgui imgui_;
    void update(float dt);
    void draw();
    void test();

    bool bucket_fill(const math::vec2i& mp, const math::vec3f& new_color);

    // gl entities.
    gl::vertex_array circle_vao;
    gl::vertex_array canvas_vao;
    gl::vertex_array resize_button_quad_vao;

    gl::named_buffer circle_vbo;
    gl::named_buffer canvas_vbo;
    gl::named_buffer resize_button_quad_vbo;

    gl::named_buffer quad_ibo;

    gl::shader circle_shader;
    gl::shader circle_batcher_shader;
    gl::shader colored_quad_shader;
    gl::shader textured_quad_shader;
    gl::shader line_shader;

    gl::sampler sampler_linear;
    gl::sampler sampler_nearest;

    gl::texture2d canvas_bg;

    math::mat4f window_projection;
    math::camera2d cam2d;

    enum class app_mode {
        DRAW = 0,
        ERASER,
        RESIZE
    } current_mode {app_mode::DRAW};

    enum class brush_type {
        PEN = 0,
        ERASER,
        BUCKET
    } current_brush_type {brush_type::PEN};

    struct draw_region {
        gl::texture2d texture;
        gl::frame_buffer buffer;

        int width  {};
        int height {};
        math::vec2f pos {};
        math::mat4f projection {};
    };

    draw_region canvas {};
    draw_region temp_canvas {};

    struct stroke {
        std::vector<math::vec2f> brush_points; // stroke control points
        float aa {};
        float brush_size {};
        math::vec3f color {};
        brush_type type;
        math::vec2i mp {}; // use only when type == bucket, this is canvas relative pixel coordinates
    };

    struct history {
        std::vector<stroke> strokes; // indexing starts from 1
        std::size_t last_stroke_index {};
        std::size_t last_valid_redo_index {};
        bool should_undo {};
        bool should_redo {};
    } stroke_history;
    
    struct info {
        std::string current_filename {""};
        std::array<float, 3> bg_color {1.0f, 1.0f, 1.0f};
        bool use_nearest {true};

        // brush stroke rendering information
        bool drawing          {false};
        bool drawing_finished {false};
        bool should_start_new_stroke {true};
        float current_brush_size {10.0f};
        float current_aa {1.0f};
        std::array<float, 3> current_color {};

        // canvas resizing information
        bool resized {false};
        bool resizing {false};
        int new_width {};
        int new_height {};
        int resize_button_index {-1};
        float resize_button_radius {7.0f};
    } info;

    std::array<math::vec2i, 8> canvas_resize_dirs {{
        {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}
    }};
};

}
