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

struct brush_point {
    math::vec2f p;
    float r {};
};

struct line {
    math::vec2f p1 {}, p2 {};
    math::vec3f color {};
    float thickness {};
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
    bool update_canvas {};
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
    void update_refactor(float dt);
    void draw_refactor();

    // gl entities.
    gl::vertex_array circle_vao;
    gl::vertex_array canvas_vao;
    gl::vertex_array line_vao;
    gl::vertex_array line_resize_vao;
    gl::vertex_array resize_button_quad_vao;

    gl::named_buffer circle_vbo;
    gl::named_buffer canvas_vbo;
    gl::named_buffer line_vbo;
    gl::named_buffer line_resize_vbo;
    gl::named_buffer resize_button_quad_vbo;

    gl::named_buffer quad_ibo;
    gl::named_buffer line_ibo;
    gl::named_buffer line_resize_ibo;

    gl::shader circle_shader;
    gl::shader circle_batcher_shader;
    gl::shader colored_quad_shader;
    gl::shader eraser_shader;
    gl::shader textured_quad_shader;
    gl::shader line_shader;

    gl::texture2d canvas_bg;

    math::mat4f window_projection;
    math::camera2d cam2d;

    enum class app_mode {
        DRAW = 0,
        RESIZE = 1
    } mode {app_mode::DRAW};

    struct draw_region {
        gl::texture2d texture;
        gl::sampler sampler;
        gl::frame_buffer buffer;

        int width  {};
        int height {};
        math::vec2f pos {};
        math::mat4f projection {};
    };

    draw_region canvas {};
    draw_region temp_canvas {};

    math::vec2f eraser_pos;

    struct brush_stroke {
        std::vector<math::vec2f> brush_points; // stroke control points
        float aa {1.0f};
        float brush_size {10.0f};
    };
    std::vector<brush_stroke> stroke_history; // indexing starts from 1
    std::size_t last_stroke_index {};
    std::size_t last_valid_redo_index {};
    bool should_undo {};
    bool should_redo {};

    //struct temp_canvas {
    //    gl::texture2d texture;
    //    gl::frame_buffer buffer;
    //    int width  {};
    //    int height {};
    //} temp_canvas;

    //struct pen_tool {
    //    std::vector<brush_point> brush_points; // for brush stroke
    //    std::array<float, 3> brush_color {};
    //    float brush_size {10.0f};
    //} pen_;

    //struct eraser {
    //    float r {10.0f};
    //} eraser_;

    struct info {
        // brush stroke rendering information
        bool drawing          {false};
        bool drawing_finished {false};
        bool new_stroke_start {true};
        float current_brush_size {5.0f};
        float current_aa {1.0f};

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

    //struct rect_selection {
    //    bool is_selecting {false};
    //    bool done {false};
    //    vec2 p1 {};
    //    vec2 p2 {};
    //} selection_info;

    //std::array<veci2, 8> resize_dirs {{
    //    {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}
    //}};

    //static constexpr int MAX_PER_BATCH {4096}; // count

    //struct line_batcher {
    //    std::vector<gl::vertex<gl::pos2, gl::color4>> lines_data;
    //    using vertex_t = typename gl::vertex<gl::pos2, gl::color4>;
    //} line_batcher;
};

/*
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

    // gl entities.
    gl::vertex_array circle_vao;
    gl::vertex_array canvas_vao;
    gl::vertex_array line_vao;
    gl::vertex_array line_resize_vao;
    gl::vertex_array resize_button_quad_vao;

    gl::named_buffer circle_vbo;
    gl::named_buffer canvas_vbo;
    gl::named_buffer line_vbo;
    gl::named_buffer line_resize_vbo;
    gl::named_buffer resize_button_quad_vbo;

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

        std::string filename {};
    } canvas, transparent_canvas;

    struct temp_canvas {
        gl::texture2d texture;
        gl::frame_buffer buffer;
        int width  {};
        int height {};
    } temp_canvas;

    struct pen_tool {
        std::vector<brush_point> brush_points; // for brush stroke
        std::array<float, 3> brush_color {};
        float brush_size {10.0f};
    } pen_;

    struct eraser {
        float r {10.0f};
    } eraser_;

    struct info {
        vec2 world_offset {};
        bool mouse_moved {};
        bool prev_mouse_moved {};
        float pan_speed {50.0f};
        float resize_speed {100.0f};
        bool should_draw {false};
        bool should_empty {false};
        bool resized {true};
        bool resizing {false};
        int new_width {};
        int new_height {};
        bool in_resize_mode {false};
        int resize_button_index {-1};
        bool in_selection_mode {false};
    } info;

    struct rect_selection {
        bool is_selecting {false};
        bool done {false};
        vec2 p1 {};
        vec2 p2 {};
    } selection_info;

    std::array<veci2, 8> resize_dirs {{
        {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}
    }};

    static constexpr int MAX_PER_BATCH {4096}; // count

    struct line_batcher {
        std::vector<gl::vertex<gl::pos2, gl::color4>> lines_data;
        using vertex_t = typename gl::vertex<gl::pos2, gl::color4>;
    } line_batcher;
};
*/

}
