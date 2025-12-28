#include "app.hpp"

#include <SDL3/SDL.h>
#include <chrono>
#include <cmath>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <print>
#include <utility>

#include "input_manager.hpp"
#include "graphics/graphics.hpp"

namespace {

constexpr bool TESTING {false};
constexpr int MAX_FPS {500};
std::vector<peria::brush_point> ps;
std::vector<peria::gl::texture2d> temp_vec; 

float zoom_scale {1.0f};

template<typename T>
[[nodiscard]]
peria::math::vec2<T> world_to_screen(const peria::math::vec2<T>& pos, const peria::math::vec2<T>& world_offset) noexcept
{ return (pos + world_offset)*zoom_scale; }

template<typename T>
[[nodiscard]]
peria::math::vec2<T> screen_to_world(const peria::math::vec2<T>& pos, const peria::math::vec2<T>& world_offset) noexcept
{ return ((pos * (1.0f/zoom_scale)) - world_offset); }

template<typename T>
[[nodiscard]]
peria::math::vec3<T> world_to_screen(const peria::math::vec3<T>& pos, const peria::math::vec3<T>& world_offset) noexcept
{ return (pos + world_offset)*zoom_scale; }

template<typename T>
[[nodiscard]]
peria::math::vec3<T> screen_to_world(const peria::math::vec3<T>& pos, const peria::math::vec3<T>& world_offset) noexcept
{ return ((pos * (1.0f/zoom_scale)) - world_offset); }

template<typename T>
[[nodiscard]]
peria::math::vec4<T> world_to_screen(const peria::math::vec4<T>& pos, const peria::math::vec4<T>& world_offset) noexcept
{ return (pos + world_offset)*zoom_scale; }

template<typename T>
[[nodiscard]]
peria::math::vec4<T> screen_to_world(const peria::math::vec4<T>& pos, const peria::math::vec4<T>& world_offset) noexcept
{ return ((pos * (1.0f/zoom_scale)) - world_offset); }

[[nodiscard]]
float lerp(float a, float b, float t) noexcept
{ return a*(1.0f - t) + b*t; }

// Generate Catmull-Rom spline which will go through each point in points based on parameter t 
[[nodiscard]]
peria::brush_point get_point_on_path(const std::vector<peria::brush_point>& points, float t)
{
    // indices of 2 control points and 2 points on a curve
    const std::size_t p1 {static_cast<std::size_t>(t)+1};
    const std::size_t p2 {p1+1};
    const std::size_t p3 {p2+1};
    const std::size_t p0 {p1-1};

    t = t - (float)static_cast<int>(t); // leftover

    const float tt {t*t};
    const float ttt {tt*t};

    // contributions based on some predefined cubic functions
    const float c1 {-ttt + 2.0f*tt - t};
    const float c2 {3.0f*ttt - 5.0f*tt + 2.0f};
    const float c3 {-3.0f*ttt + 4.0f*tt + t};
    const float c4 {ttt-tt};

    const float r {lerp(points[p1].r, points[p2].r, t)};

    return peria::brush_point {
        0.5f * vec2{
            (c1*points[p0].p.x + c2*points[p1].p.x + c3*points[p2].p.x + c4*points[p3].p.x),
            (c1*points[p0].p.y + c2*points[p1].p.y + c3*points[p2].p.y + c4*points[p3].p.y)
        }, r
    };
}

[[nodiscard]]
std::vector<peria::gl::vertex<peria::gl::pos2, peria::gl::color4>> generic_add_line(const peria::line& line)
{
    const float vec_x {line.p2.x-line.p1.x};
    const float vec_y {line.p2.y-line.p1.y};
    const float len {std::sqrt(vec_x*vec_x+vec_y*vec_y)};
    const float dir_x {vec_x/len};
    const float dir_y {vec_y/len};
    const float dir_x_90 {-dir_y};
    const float dir_y_90 {dir_x};

    const float lower_left_x {line.p1.x - line.thickness*dir_x_90};
    const float lower_left_y {line.p1.y - line.thickness*dir_y_90};

    const float upper_left_x {line.p1.x + line.thickness*dir_x_90};
    const float upper_left_y {line.p1.y + line.thickness*dir_y_90};

    const float upper_right_x {line.p2.x + line.thickness*dir_x_90};
    const float upper_right_y {line.p2.y + line.thickness*dir_y_90};

    const float lower_right_x {line.p2.x - line.thickness*dir_x_90};
    const float lower_right_y {line.p2.y - line.thickness*dir_y_90};

    const auto& [r, g, b] {line.color};
    return {
        {{lower_left_x,  lower_left_y }, {r, g, b, 1.0f}},
        {{upper_left_x,  upper_left_y }, {r, g, b, 1.0f}},
        {{upper_right_x, upper_right_y}, {r, g, b, 1.0f}},
        {{lower_right_x, lower_right_y}, {r, g, b, 1.0f}}
    };
}

}

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

namespace imgui {

// SDL3 is already initialized together with window, glContext and glad
imgui::imgui(SDL_Window* window, SDL_GLContext gl_context, const char* glsl_version) noexcept
{
    std::println("imgui init");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

imgui::~imgui()
{
    std::println("imgui shutdown");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

bool imgui::is_imgui_captured() noexcept
{ return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard; }

bool imgui::is_imgui_hovered() noexcept
{ return ImGui::GetIO().MouseHoveredViewport; };

} // end imgui

application::application(application_settings&& settings)
    :app_settings_{std::move(settings)},
     sdl_initializer_{app_settings_},
     imgui_{sdl_initializer_.window, sdl_initializer_.context, "#version 460"},
     circle_shader{"./assets/shaders/circle.vert", "./assets/shaders/circle.frag"},
     textured_quad_shader{"./assets/shaders/quad.vert", "./assets/shaders/quad.frag"},
     line_shader{"./assets/shaders/line_v2.vert", "./assets/shaders/line_v2.frag"},
     canvas{gl::texture2d{graphics::create_texture2d(static_cast<int>(settings.window_width*0.75f), static_cast<int>(settings.window_height*0.75f), GL_RGBA16F)},
            gl::sampler{graphics::create_sampler(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER)}, {}, 
            static_cast<int>(settings.window_width*0.75f), static_cast<int>(settings.window_height*0.75f), {}, {}, graphics::WHITE},
     temp_canvas{{gl::texture2d{graphics::create_texture2d(canvas.width, canvas.height, GL_RGBA16F)}}, {}, canvas.width, canvas.height}
{
    if (!sdl_initializer_.initialized) return;
    std::println("application construction");
    int mx_sz {};
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mx_sz);
    std::println("MAX TEX SIZE {}", mx_sz);

    input_manager::initialize();
    temp_vec.reserve(100);
    {
        peria::graphics::set_vsync(true);
        peria::graphics::set_relative_mouse(sdl_initializer_.window, false);
        peria::graphics::set_screen_size(app_settings_.window_width, app_settings_.window_height);
        window_projection = math::get_ortho_projection(0.0f, static_cast<float>(app_settings_.window_width), 0.0f, static_cast<float>(app_settings_.window_height));
    }

    // GL entities here
    {
        std::array<u32, 6> indices {0,1,2, 0,2,3};

        std::array<gl::vertex<gl::pos2>, 4> circle_data {{
            {{-0.5f, -0.5f}},
            {{-0.5f,  0.5f}},
            {{ 0.5f,  0.5f}},
            {{ 0.5f, -0.5f}}
        }};
        graphics::buffer_upload_data(circle_vbo, circle_data, GL_STATIC_DRAW);
        graphics::buffer_upload_data(quad_ibo, indices, GL_STATIC_DRAW);
        graphics::vao_configure<gl::pos2>(circle_vao, circle_vbo, 0);
        graphics::vao_connect_ibo(circle_vao, quad_ibo);

        std::array<gl::vertex<gl::pos2, gl::texture_coord>, 4> canvas_data {{
            {{-0.5f, -0.5f}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f}, {0.0f, 1.0f}},
            {{ 0.5f,  0.5f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f}, {1.0f, 0.0f}},
        }};
        graphics::buffer_upload_data(canvas_vbo, canvas_data, GL_STATIC_DRAW);
        graphics::vao_configure<gl::pos2, gl::texture_coord>(canvas_vao, canvas_vbo, 0);
        graphics::vao_connect_ibo(canvas_vao, quad_ibo);

        {
            std::array<u32, MAX_PER_BATCH*6> line_indices;
            for (int i{}, k{}; i<MAX_PER_BATCH*6; i+=6, k+=4) {
                line_indices[static_cast<std::size_t>(i)] = static_cast<u32>(k);
                line_indices[static_cast<std::size_t>(i+1)] = static_cast<u32>(k+1);
                line_indices[static_cast<std::size_t>(i+2)] = static_cast<u32>(k+2);

                line_indices[static_cast<std::size_t>(i+3)] = static_cast<u32>(k);
                line_indices[static_cast<std::size_t>(i+4)] = static_cast<u32>(k+2);
                line_indices[static_cast<std::size_t>(i+5)] = static_cast<u32>(k+3);
            }

            graphics::buffer_allocate_data(line_vbo, MAX_PER_BATCH*4*gl::vertex<gl::pos2, gl::color4>::stride, GL_DYNAMIC_DRAW);
            graphics::buffer_upload_data(line_ibo, line_indices, GL_STATIC_DRAW);
            graphics::vao_configure<gl::pos2, gl::color4>(line_vao, line_vbo, 0);
            graphics::vao_connect_ibo(line_vao, line_ibo);
        }

        {
            constexpr int line_count {4};
            std::array<u32, line_count*6> line_indices;
            for (int i{}, k{}; i<line_count*6; i+=6, k+=4) {
                line_indices[static_cast<std::size_t>(i)] = static_cast<u32>(k);
                line_indices[static_cast<std::size_t>(i+1)] = static_cast<u32>(k+1);
                line_indices[static_cast<std::size_t>(i+2)] = static_cast<u32>(k+2);

                line_indices[static_cast<std::size_t>(i+3)] = static_cast<u32>(k);
                line_indices[static_cast<std::size_t>(i+4)] = static_cast<u32>(k+2);
                line_indices[static_cast<std::size_t>(i+5)] = static_cast<u32>(k+3);
            }
            graphics::buffer_allocate_data(line_resize_vbo, line_count*4*gl::vertex<gl::pos2, gl::color4>::stride, GL_DYNAMIC_DRAW);
            graphics::buffer_upload_data(line_resize_ibo, line_indices, GL_STATIC_DRAW);
            graphics::vao_configure<gl::pos2, gl::color4>(line_resize_vao, line_resize_vbo, 0);
            graphics::vao_connect_ibo(line_resize_vao, line_resize_ibo);
        }

        {
            std::array<gl::vertex<gl::pos2, gl::color4>, 4> resize_button_quad {{
                {{-0.5f, -0.5f}, {0.0f, 0.0f, 0.5f, 1.0f}},
                {{-0.5f,  0.5f}, {0.0f, 0.0f, 0.5f, 1.0f}},
                {{ 0.5f,  0.5f}, {0.0f, 0.0f, 0.5f, 1.0f}},
                {{ 0.5f, -0.5f}, {0.0f, 0.0f, 0.5f, 1.0f}}
            }};
            graphics::buffer_upload_data(resize_button_quad_vbo, resize_button_quad, GL_STATIC_DRAW);
            graphics::vao_configure<gl::pos2, gl::color4>(resize_button_quad_vao, resize_button_quad_vbo, 0);
            graphics::vao_connect_ibo(resize_button_quad_vao, quad_ibo);
        }


        // CANVAS FRAME BUFFER AND COLOR ATTACHMENTS
        {
            glNamedFramebufferTexture(canvas.buffer.id, GL_COLOR_ATTACHMENT0, canvas.texture.id, 0);
            auto status {glCheckNamedFramebufferStatus(canvas.buffer.id, GL_FRAMEBUFFER)};
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                std::println("FrameBuffer with id {} is incomplete\n {}", canvas.buffer.id, status);
            }
            glNamedFramebufferTexture(temp_canvas.buffer.id, GL_COLOR_ATTACHMENT0, temp_canvas.texture.id, 0);
            status = glCheckNamedFramebufferStatus(temp_canvas.buffer.id, GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                std::println("FrameBuffer with id {} is incomplete\n {}", temp_canvas.buffer.id, status);
            }
        }

        textured_quad_shader.set_int("u_canvas_texture", 0);
        canvas.projection = math::get_ortho_projection(0.0f, static_cast<float>(canvas.width), 0.0f, static_cast<float>(canvas.height));
        canvas.pos = 0.5f*vec2{static_cast<float>(graphics::get_screen_size().w), static_cast<float>(graphics::get_screen_size().h)}; 
        info.new_width = canvas.width;
        info.new_height = canvas.height;
    }

    pen_.brush_points.reserve(2048);
    // times 4 because each line is represented as a quad which is 2 triangles with total of 4 verts
    line_batcher.lines_data.reserve(MAX_PER_BATCH*4);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // just clear canvas to default color before doing anything
    graphics::bind_frame_buffer(canvas.buffer);
    graphics::set_viewport(0, 0, canvas.width, canvas.height);
    graphics::clear_buffer_color(canvas.buffer.id, canvas.bg_color);
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

    std::chrono::steady_clock clock {};
    auto prev_time {clock.now()};

    while (running) {
        const auto current_time {clock.now()};
        auto elapsed_time {std::chrono::duration_cast<std::chrono::milliseconds>(current_time-prev_time).count()};
    
        // milliseconds
        if (const auto t{(1.0f/MAX_FPS)*1000.0f}; static_cast<float>(elapsed_time) < t) {
            SDL_Delay(static_cast<uint32_t>(t-static_cast<float>(elapsed_time)));
            elapsed_time = static_cast<uint32_t>(t);
        }

        prev_time = current_time;
        const auto dt {static_cast<float>(elapsed_time)*0.001f}; // in seconds

        input_manager_->update_mouse();

        // Poll for events, and react to window resize and mouse movement events here
        for (SDL_Event ev; SDL_PollEvent(&ev);) {
            ImGui_ImplSDL3_ProcessEvent(&ev);

            if (ev.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }
            else if (ev.type == SDL_EVENT_WINDOW_RESIZED) {
                app_settings_.window_width = ev.window.data1;
                app_settings_.window_height = ev.window.data2;
                graphics::set_viewport(0, 0, app_settings_.window_width, app_settings_.window_height);
                graphics::set_screen_size(app_settings_.window_width, app_settings_.window_height);
                window_projection = math::get_ortho_projection(0.0f, static_cast<float>(app_settings_.window_width), 0.0f, static_cast<float>(app_settings_.window_height));
            }
            else if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                peria::graphics::set_relative_motion(ev.motion.xrel, -ev.motion.yrel);
                info.mouse_moved = true;
            }
            else if (ev.type == SDL_EVENT_MOUSE_WHEEL && imgui_.update_canvas) {
                const auto [mx, my] {input_manager_->get_mouse_gl()};
                auto mouse_world {screen_to_world({mx, my}, info.world_offset)};

                if (ev.wheel.y > 0.0f) {
                    zoom_scale *= 2.0f;
                    zoom_scale = std::min(zoom_scale, 512.0f);
                }
                if (ev.wheel.y < 0.0f) {
                    zoom_scale *= 0.5f;
                    zoom_scale = std::max(zoom_scale, 0.01f);
                }
                auto mouse_world_after_zoom {screen_to_world({mx, my}, info.world_offset)};

                info.world_offset += (mouse_world_after_zoom - mouse_world);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        imgui_.update_canvas = !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow); 
        
        update(dt);
        draw();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::Button("save")) {
                graphics::write_to_png(canvas.texture, canvas.width, canvas.height);
            }
            if (ImGui::Button("tools")) {
                if (imgui_.show_tools) imgui_.show_tools = false;
                else                   imgui_.show_tools = true;
            }
        }
        ImGui::EndMainMenuBar();

        if (imgui_.show_tools) {
            if (ImGui::Begin("tools")) {
                ImGui::ColorPicker3("pen_brush_color", pen_.brush_color.data());
                ImGui::SliderFloat("pen_brush_size", &pen_.brush_size, 1.0f, static_cast<float>(std::min(canvas.width, canvas.height))/4.0f);
                ImGui::SliderFloat("eraser_radius", &eraser_.r, 1.0f, static_cast<float>(std::min(canvas.width, canvas.height))/4.0f);
                if (ImGui::Button("reset")) {
                    info.world_offset = {0, 0};
                    zoom_scale = 1.0f;
                }
                if (ImGui::Checkbox("pen_brush", &imgui_.pen_selected)) {
                    imgui_.eraser_selected = !imgui_.pen_selected;
                }
                if (ImGui::Checkbox("eraser", &imgui_.eraser_selected)) {
                    imgui_.pen_selected = !imgui_.eraser_selected;
                }
            }
            ImGui::End();
        }
            
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(sdl_initializer_.window);
        info.prev_mouse_moved = info.mouse_moved;
        info.mouse_moved = false;
        
        {
            std::string title {"peria_paint | "+std::to_string(elapsed_time)+" "+std::to_string(dt)};
            if (graphics::is_vsync()) title += " VSYNC: ON";
            else title += " VSYNC: OFF";

            SDL_SetWindowTitle(sdl_initializer_.window, title.c_str());
        }

        input_manager_->update_prev_state();
    }
}

void application::update(float dt)
{
    const auto im {input_manager::instance()};
    const auto [mx, my] {im->get_mouse_gl()};
    info.should_draw = false;

    if (im->key_down(SDL_SCANCODE_LCTRL) && im->key_pressed(SDL_SCANCODE_S)) {
        graphics::write_to_png(canvas.texture, canvas.width, canvas.height);
    }

    if (im->key_pressed(SDL_SCANCODE_V)) {
        graphics::set_vsync(!graphics::is_vsync());
    }

    if (im->key_pressed(SDL_SCANCODE_T)) {
        if (imgui_.show_tools) imgui_.show_tools = false;
        else                   imgui_.show_tools = true;
    }

    if (im->key_pressed(SDL_SCANCODE_B)) {
        imgui_.pen_selected = true;
        imgui_.eraser_selected = false;
    }

    if (im->key_pressed(SDL_SCANCODE_E)) {
        imgui_.eraser_selected = true;
        imgui_.pen_selected = false;
    }

    if (im->key_pressed(SDL_SCANCODE_R)) {
        if (info.in_resize_mode) info.in_resize_mode = false;
        else                     info.in_resize_mode = true;
    }

    if (!imgui_.update_canvas) {
        info.should_draw = true;
        info.should_empty = true;
        return;
    }

    // panning around
    if (info.mouse_moved && im->mouse_down(mouse_button::MID)) {
        const auto rel_motion {graphics::get_relative_motion()};
        info.world_offset.x += (rel_motion.x/zoom_scale);
        info.world_offset.y += (rel_motion.y/zoom_scale);
        return; // we don't want to draw while panning around
    }

    {
        const auto canvas_world_center_x {canvas.pos.x};
        const auto canvas_world_center_y {canvas.pos.y};
        const auto canvas_lower_left_x   {canvas_world_center_x-canvas.width*0.5f};
        const auto canvas_lower_left_y   {canvas_world_center_y-canvas.height*0.5f};
        const auto canvas_upper_right_x  {canvas_world_center_x+canvas.width*0.5f};
        const auto canvas_upper_right_y  {canvas_world_center_y+canvas.height*0.5f};

        const auto world_mpos {screen_to_world({mx, my}, info.world_offset)};
        bool inside_canvas {world_mpos.x >= canvas_lower_left_x &&
                            world_mpos.x <= canvas_upper_right_x &&
                            world_mpos.y >= canvas_lower_left_y &&
                            world_mpos.y <= canvas_upper_right_y};

        auto mouse_inside_button = [this, &world_mpos]() -> int {
            constexpr int resize_button_len {15};
            int k {};
            for (const auto& [dx, dy]:resize_dirs) {
                const vec2 button_world_pos {
                    canvas.pos.x+(0.5f*canvas.width*dx),
                    canvas.pos.y+(0.5f*canvas.height*dy),
                };
                const auto button_lower_left_x   {button_world_pos.x-resize_button_len*0.5f};
                const auto button_lower_left_y   {button_world_pos.y-resize_button_len*0.5f};
                const auto button_upper_right_x  {button_world_pos.x+resize_button_len*0.5f};
                const auto button_upper_right_y  {button_world_pos.y+resize_button_len*0.5f};
                bool inside_button {world_mpos.x >= button_lower_left_x &&
                                    world_mpos.x <= button_upper_right_x &&
                                    world_mpos.y >= button_lower_left_y &&
                                    world_mpos.y <= button_upper_right_y};
                if (inside_button) {
                    return k;
                }
                ++k;
            }
            return -1;
        };

        if (info.in_resize_mode) {
            if (!info.resizing) {
                // determine if we are resizing, and from which button
                const auto index {mouse_inside_button()};
                std::println("We are inside button {}", index);
                if(index != -1 && im->mouse_pressed(mouse_button::LEFT)) {
                    info.resizing = true;
                    info.resize_button_index = index;
                }
            }
            if (info.resizing && im->mouse_down(mouse_button::LEFT)) {
                // REMOVE ME LATER
                //const auto rel_motion {graphics::get_relative_motion()};
                //const auto dx {rel_motion.x*(resize_dirs[info.resize_button_index].x)};
                //const auto dy {rel_motion.y*(resize_dirs[info.resize_button_index].y)};
                //info.new_width  += static_cast<int>(dx);
                //info.new_height += static_cast<int>(dy);

                const auto dx {world_mpos.x-(canvas.pos.x+resize_dirs[info.resize_button_index].x*canvas.width*0.5f)};
                const auto dy {world_mpos.y-(canvas.pos.y+resize_dirs[info.resize_button_index].y*canvas.height*0.5f)};

                info.new_width  = static_cast<int>(canvas.width+dx*resize_dirs[info.resize_button_index].x);
                info.new_height = static_cast<int>(canvas.height+dy*resize_dirs[info.resize_button_index].y);

                //std::println("rel motion {} {}, index {}", dx, dy, info.resize_button_index);
            }
            if (info.resizing && im->mouse_released(mouse_button::LEFT)) {
                temp_canvas.texture = graphics::create_texture2d(info.new_width, info.new_height, GL_RGBA16F);
                glNamedFramebufferTexture(temp_canvas.buffer.id, GL_COLOR_ATTACHMENT0, temp_canvas.texture.id, 0);
                const auto status {glCheckNamedFramebufferStatus(temp_canvas.buffer.id, GL_FRAMEBUFFER)};
                if (status != GL_FRAMEBUFFER_COMPLETE) {
                    std::println("FrameBuffer with id {} is incomplete\n {}", temp_canvas.buffer.id, status);
                }
                info.resizing = false;
                info.resized = true;
            }
        } 
        else {
            // brush stroke related stuff here
            // only add brush strokes and do drawing if not in resize mode
            if (im->mouse_released(mouse_button::LEFT) && inside_canvas && !info.mouse_moved) {
                pen_.brush_points.emplace_back(brush_point{{world_mpos.x-canvas_lower_left_x, world_mpos.y-canvas_lower_left_y}, pen_.brush_size*0.5f});
                info.should_draw = true;
                info.should_empty = true;
                return;
            }

            // TODO: Fix this shit. This code is about holding mouse while drawing
            // and going outside of canvas stuff.
            if ((info.prev_mouse_moved || info.mouse_moved) && !inside_canvas) {
                if (pen_.brush_points.size() >= 2) {
                    const auto idx {pen_.brush_points.size() - 1};
                    auto dir_x {pen_.brush_points[idx].p.x - pen_.brush_points[idx-1].p.x};
                    auto dir_y {pen_.brush_points[idx].p.y - pen_.brush_points[idx-1].p.y};
                    const auto len {std::sqrtf(dir_x*dir_x + dir_y*dir_y)};
                    dir_x /= len;
                    dir_y /= len;
                    const auto m {std::max(canvas.width, canvas.height)};
                    pen_.brush_points.emplace_back(brush_point{{pen_.brush_points[idx].p.x+dir_x*m, pen_.brush_points[idx].p.y+dir_y*m}, pen_.brush_size*0.5f});
                }

                info.should_draw = true;
                info.should_empty = true;
                return;
            }

            if (info.mouse_moved) {
                if (im->mouse_down(mouse_button::LEFT) && inside_canvas) {
                    pen_.brush_points.emplace_back(brush_point{{world_mpos.x-canvas_lower_left_x, world_mpos.y-canvas_lower_left_y}, pen_.brush_size*0.5f});
                    info.should_draw = true;
                    info.should_empty = false;
                }

                if (im->mouse_released(mouse_button::LEFT) && inside_canvas) {
                    pen_.brush_points.emplace_back(brush_point{{world_mpos.x-canvas_lower_left_x, world_mpos.y-canvas_lower_left_y}, pen_.brush_size*0.5f});
                    info.should_draw = true;
                    info.should_empty = true;
                }
            }
        }
    }
}

void application::draw()
{
    // Clear resized buffer and copy all contents from old to new.
    // Then swap struct info as well.
    if (info.resized) {
        temp_canvas.width = info.new_width;
        temp_canvas.height = info.new_height;
        // I do need these to clear to BG color.
        // Or we can manually update pixels on cpu. Kinda annoying and need to be careful
        // with texture format
        graphics::bind_frame_buffer(temp_canvas.buffer);
        graphics::set_viewport(0, 0, temp_canvas.width, temp_canvas.height);
        graphics::clear_buffer_color(temp_canvas.buffer.id, canvas.bg_color);

        const auto w {std::min(temp_canvas.width, canvas.width)};
        const auto h {std::min(temp_canvas.height, canvas.height)};

        int dst_x {};
        int dst_y {};
        int src_x {};
        int src_y {};
        int dw {info.new_width-canvas.width};
        int dh {info.new_height-canvas.height};
        switch (info.resize_button_index) {
            case 0:
                if (dw > 0) dst_x = dw;
                else        src_x = -dw;
                if (dh > 0) dst_y = dh;
                else        src_y = -dh;
                canvas.pos += vec2{-dw*0.5f, -dh*0.5f};
                break;
            case 1: 
            case 2:
                if (dw > 0) dst_x = dw;
                else        src_x = -dw;
                canvas.pos += vec2{-dw*0.5f, dh*0.5f};
                break;
            case 3:
            case 4:
            case 5:
                canvas.pos += vec2{dw*0.5f, dh*0.5f};
                break;
            case 6: 
            case 7:
                if (dh > 0) dst_y = dh;
                else        src_y = -dh;
                canvas.pos += vec2{dw*0.5f, -dh*0.5f};
                break;
            default:
                break;
        }

        glCopyImageSubData(canvas.texture.id, GL_TEXTURE_2D , 0, src_x, src_y, 0,
                           temp_canvas.texture.id, GL_TEXTURE_2D, 0, dst_x, dst_y, 0, 
                           w, h, 1);

        //std::println("ids {} {}", canvas.buffer.id, temp_canvas.buffer.id);

        std::swap(temp_canvas.buffer.id, canvas.buffer.id);
        std::swap(temp_canvas.texture.id, canvas.texture.id);
        std::swap(temp_canvas.width, canvas.width);
        std::swap(temp_canvas.height, canvas.height);
        canvas.projection = math::get_ortho_projection(0.0f, static_cast<float>(canvas.width), 0.0f, static_cast<float>(canvas.height));

        info.resized = false;
        info.resize_button_index = -1;
    }

    // Only draw if we are doing brush strokes.
    if (info.should_draw) {
        graphics::bind_frame_buffer(canvas.buffer);
        graphics::set_viewport(0, 0, canvas.width, canvas.height);

        graphics::bind_vertex_array(circle_vao);
        circle_shader.use_shader();

        {
            auto [r, g, b] {pen_.brush_color};
            if (imgui_.eraser_selected) {
                r = canvas.bg_color.r;
                g = canvas.bg_color.g;
                b = canvas.bg_color.b;
            }
            circle_shader.set_vec4("u_color", vec4{r, g, b, 1.0f});
        }

        // Draw brush stroke points
        for (std::size_t i{}; i<pen_.brush_points.size(); ++i) {
            const auto& pos {pen_.brush_points[i].p};
            auto r {pen_.brush_points[i].r};
            if (imgui_.eraser_selected) r = eraser_.r;
            math::mat4f m {math::translate(pos.x, pos.y, 0.0f)*
                           math::scale(2*r, 2*r, 1.0f)};
            circle_shader.set_mat4("u_mvp", canvas.projection*m);
            circle_shader.set_float("u_radius", r);
            circle_shader.set_vec2("u_center_world", pos.x, pos.y);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        std::vector<brush_point> samples;
        for (float t{}; t<static_cast<float>(pen_.brush_points.size())-3.0f; t+=0.2f) {
            samples.emplace_back(get_point_on_path(pen_.brush_points, t));
        }

        // Draw interpolated sample points on stroke points
        for (std::size_t i{}; i<samples.size(); ++i) {
            auto r {samples[i].r};
            if (imgui_.eraser_selected) r = eraser_.r;
            math::mat4f m {math::translate(samples[i].p.x, samples[i].p.y, 0.0f)*
                           math::scale(r*2, r*2, 1.0f)};
            circle_shader.set_mat4("u_mvp", canvas.projection*m);
            circle_shader.set_float("u_radius", r);
            circle_shader.set_vec2("u_center_world", samples[i].p.x, samples[i].p.y);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        // Draw lines between interpolated points

        // DRAWS LINES BETWEEN MOUSE POINTS

        auto add_line = [this](const brush_point& p1, const brush_point& p2) {
            const auto& x1 {p1.p.x};
            const auto& y1 {p1.p.y};
            const auto& t1 {p1.r};
            const auto& x2 {p2.p.x};
            const auto& y2 {p2.p.y};
            const auto& t2 {p2.r};

            const float vec_x {x2-x1};
            const float vec_y {y2-y1};
            const float len {std::sqrt(vec_x*vec_x+vec_y*vec_y)};
            const float dir_x {vec_x/len};
            const float dir_y {vec_y/len};
            const float dir_x_90 {-dir_y};
            const float dir_y_90 {dir_x};

            const float lower_left_x {x1 - t1*dir_x_90};
            const float lower_left_y {y1 - t1*dir_y_90};

            const float upper_left_x {x1 + t1*dir_x_90};
            const float upper_left_y {y1 + t1*dir_y_90};

            const float upper_right_x {x2 + t2*dir_x_90};
            const float upper_right_y {y2 + t2*dir_y_90};

            const float lower_right_x {x2 - t2*dir_x_90};
            const float lower_right_y {y2 - t2*dir_y_90};

            auto [r, g, b] {pen_.brush_color};
            if (imgui_.eraser_selected) {
                r = canvas.bg_color.r;
                g = canvas.bg_color.g;
                b = canvas.bg_color.b;
            }
            line_batcher.lines_data.push_back({{lower_left_x,  lower_left_y }, {r, g, b, 1.0f}});
            line_batcher.lines_data.push_back({{upper_left_x,  upper_left_y }, {r, g, b, 1.0f}});
            line_batcher.lines_data.push_back({{upper_right_x, upper_right_y}, {r, g, b, 1.0f}});
            line_batcher.lines_data.push_back({{lower_right_x, lower_right_y}, {r, g, b, 1.0f}});
        };

        for (std::size_t i{1}; i<samples.size(); ++i) {
            const auto& x1 {samples[i-1].p.x};
            const auto& y1 {samples[i-1].p.y};
            const auto& t1 {samples[i-1].r};
            const auto& x2 {samples[i].p.x};
            const auto& y2 {samples[i].p.y};
            const auto& t2 {samples[i].r};

            const float vec_x {x2-x1};
            const float vec_y {y2-y1};
            const float len {std::sqrt(vec_x*vec_x+vec_y*vec_y)};
            const float dir_x {vec_x/len};
            const float dir_y {vec_y/len};
            const float dir_x_90 {-dir_y};
            const float dir_y_90 {dir_x};

            const float lower_left_x {x1 - t1*dir_x_90};
            const float lower_left_y {y1 - t1*dir_y_90};

            const float upper_left_x {x1 + t1*dir_x_90};
            const float upper_left_y {y1 + t1*dir_y_90};

            const float upper_right_x {x2 + t2*dir_x_90};
            const float upper_right_y {y2 + t2*dir_y_90};

            const float lower_right_x {x2 - t2*dir_x_90};
            const float lower_right_y {y2 - t2*dir_y_90};

            auto [r, g, b] {pen_.brush_color};
            if (imgui_.eraser_selected) {
                r = canvas.bg_color.r;
                g = canvas.bg_color.g;
                b = canvas.bg_color.b;
            }
            line_batcher.lines_data.push_back({{lower_left_x,  lower_left_y }, {r, g, b, 1.0f}});
            line_batcher.lines_data.push_back({{upper_left_x,  upper_left_y }, {r, g, b, 1.0f}});
            line_batcher.lines_data.push_back({{upper_right_x, upper_right_y}, {r, g, b, 1.0f}}); 
            line_batcher.lines_data.push_back({{lower_right_x, lower_right_y}, {r, g, b, 1.0f}});
        }
        if (pen_.brush_points.size() >= 2 && samples.size() >= 2) {
            add_line({{pen_.brush_points[0].p.x, pen_.brush_points[0].p.y}, pen_.brush_points[0].r}, samples[0]);
            add_line(samples.back(), {{pen_.brush_points.back().p.x, pen_.brush_points.back().p.y}, pen_.brush_points.back().r});
        }

        graphics::bind_vertex_array(line_vao);
        line_shader.use_shader();
        line_shader.set_mat4("u_mvp", canvas.projection);

        int line_count {static_cast<int>((line_batcher.lines_data.size()/4))};
        int iterations {line_count / MAX_PER_BATCH};
        int leftover   {line_count - (MAX_PER_BATCH*iterations)};
        for (int i{}; i<iterations; ++i) {
            graphics::buffer_upload_subdata(line_vbo, 0, 4*MAX_PER_BATCH*line_batcher::vertex_t::stride, line_batcher.lines_data.data()+i*MAX_PER_BATCH*4);
            glDrawElements(GL_TRIANGLES, MAX_PER_BATCH*6, GL_UNSIGNED_INT, nullptr);
        }
        if (leftover > 0) {
            graphics::buffer_upload_subdata(line_vbo, 0, 4*static_cast<u32>(leftover)*line_batcher::vertex_t::stride, line_batcher.lines_data.data()+iterations*MAX_PER_BATCH*4);
            glDrawElements(GL_TRIANGLES, leftover*6, GL_UNSIGNED_INT, nullptr);
        }
        line_batcher.lines_data.clear();
        if (pen_.brush_points.size() > 1024) pen_.brush_points.clear(); // INVESTIGATE THIS!!!!

        if (info.should_empty) {
            pen_.brush_points.clear();
        }

    }

    // Last pass to render canvas on a quad and position it in the world based on world pos and world offset.
    {
        graphics::bind_frame_buffer_default();
        graphics::clear_buffer_all(0, graphics::GREY, 1.0f, 0);
        graphics::set_viewport(0, 0, app_settings_.window_width, app_settings_.window_height);

        const auto cw {static_cast<float>(canvas.width)};
        const auto ch {static_cast<float>(canvas.height)};

        // I guess more appropriate name would be world to view??
        // In 2D panning and zooming is kinda different compared to 3D.
        // camera zooming and panning is incorporated in this model matrix.
        const auto pos {world_to_screen(canvas.pos, info.world_offset)};
        math::mat4f model {math::translate(pos.x, pos.y, 0.0f)*
                           math::scale(zoom_scale*cw, zoom_scale*ch, 1.0f)};

        graphics::bind_vertex_array(canvas_vao);
        textured_quad_shader.use_shader();
        textured_quad_shader.set_mat4("u_mvp", window_projection*model);
        textured_quad_shader.set_float("u_temp_toggle", 1.0f);
        graphics::bind_texture_and_sampler(canvas.texture, canvas.sampler, 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // draw brush/eraser outline
        {
            const auto canvas_world_center_x {canvas.pos.x};
            const auto canvas_world_center_y {canvas.pos.y};
            const auto canvas_lower_left_x   {canvas_world_center_x-canvas.width*0.5f};
            const auto canvas_lower_left_y   {canvas_world_center_y-canvas.height*0.5f};
            const auto canvas_upper_right_x  {canvas_world_center_x+canvas.width*0.5f};
            const auto canvas_upper_right_y  {canvas_world_center_y+canvas.height*0.5f};

            const auto [mx, my] {input_manager::instance()->get_mouse_gl()};
            const auto world_mpos {screen_to_world({mx, my}, info.world_offset)};
            bool inside_canvas {world_mpos.x >= canvas_lower_left_x &&
                                world_mpos.x <= canvas_upper_right_x &&
                                world_mpos.y >= canvas_lower_left_y &&
                                world_mpos.y <= canvas_upper_right_y};

            if (inside_canvas && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
                graphics::bind_vertex_array(circle_vao);
                circle_shader.use_shader();
                
                auto r {pen_.brush_size*0.5f*zoom_scale};
                if (imgui_.eraser_selected) r = eraser_.r*zoom_scale;
                math::mat4f m {math::translate(mx, my, 0.0f)*
                               math::scale(r*2, r*2, 1.0f)};
                circle_shader.set_mat4("u_mvp", window_projection*m);
                circle_shader.set_float("u_radius", r);
                circle_shader.set_vec2("u_center_world", mx, my);
                circle_shader.set_vec4("u_color", vec4{1.0f, 0.0f, 1.0f, 1.0f});
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            }
        }
        
        if (info.in_resize_mode) {
            graphics::bind_vertex_array(canvas_vao);
            // line shader is basically colored quad shader, RENAME later
            line_shader.use_shader();
            constexpr int resize_button_len {15};
            for (const auto& [dx, dy]:resize_dirs) {
                math::mat4f quad_model {math::translate(pos.x+(0.5f*cw*zoom_scale*dx), pos.y+(0.5f*ch*zoom_scale*dy), 0.0f)*
                                        math::scale(zoom_scale*resize_button_len, zoom_scale*resize_button_len, 1.0f)};
                line_shader.set_mat4("u_mvp", window_projection*quad_model);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            }
        }

        if (info.resizing) {
            graphics::bind_vertex_array(line_vao);
            line_shader.use_shader();
            line_shader.set_mat4("u_mvp", window_projection);

            const auto offset_x {info.new_width-canvas.width};
            const auto offset_y {info.new_height-canvas.height};

            std::vector<line> resize_lines; resize_lines.reserve(4);
            switch (info.resize_button_index) {
                case 0: {
                    const auto p1 {vec2{pos.x-(cw*0.5f+offset_x)*zoom_scale, pos.y-(ch*0.5f+offset_y)*zoom_scale}};
                    const auto p2 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y-(ch*0.5f+offset_y)*zoom_scale}};
                    const auto p3 {vec2{pos.x-(cw*0.5f+offset_x)*zoom_scale, pos.y+ch*0.5f*zoom_scale}};

                    resize_lines.emplace_back(line{p1, p2, {}, 1.0f});
                    resize_lines.emplace_back(line{p1, p3, {}, 1.0f});
                    if (offset_x > 0) {
                        const auto p4 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y+ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p4, p3, {}, 1.0f});
                    }
                    if (offset_y > 0) {
                        const auto p4 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y-ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p4, p2, {}, 1.0f});
                    }
                } break;
                case 1: {
                    const auto p1 {vec2{pos.x-(cw*0.5f+offset_x)*zoom_scale, pos.y+ch*0.5f*zoom_scale}};
                    const auto p2 {vec2{pos.x-(cw*0.5f+offset_x)*zoom_scale, pos.y-ch*0.5f*zoom_scale}};
                    resize_lines.emplace_back(line{p1, p2, {}, 1.0f});
                    if (offset_x > 0) {
                        const auto p3 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y+ch*0.5f*zoom_scale}};
                        const auto p4 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y-ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p1, p3, {}, 1.0f});
                        resize_lines.emplace_back(line{p2, p4, {}, 1.0f});
                    }
                } break;
                case 2: {
                    const auto p1 {vec2{pos.x-(cw*0.5f+offset_x)*zoom_scale, pos.y+(ch*0.5f+offset_y)*zoom_scale}};
                    const auto p2 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y+(ch*0.5f+offset_y)*zoom_scale}};
                    const auto p3 {vec2{pos.x-(cw*0.5f+offset_x)*zoom_scale, pos.y-ch*0.5f*zoom_scale}};

                    resize_lines.emplace_back(line{p1, p2, {}, 1.0f});
                    resize_lines.emplace_back(line{p1, p3, {}, 1.0f});
                    if (offset_x > 0) {
                        const auto p4 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y-ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p4, p3, {}, 1.0f});
                    }
                    if (offset_y > 0) {
                        const auto p4 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y+ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p4, p2, {}, 1.0f});
                    }
                } break;
                case 3: {
                    const auto p1 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y+(ch*0.5f+offset_y)*zoom_scale}};
                    const auto p2 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y+(ch*0.5f+offset_y)*zoom_scale}};
                    resize_lines.emplace_back(line{p1, p2, {}, 1.0f});
                    if (offset_y > 0) {
                        const auto p3 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y+ch*0.5f*zoom_scale}};
                        const auto p4 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y+ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p3, p1, {}, 1.0f});
                        resize_lines.emplace_back(line{p4, p2, {}, 1.0f});
                    }
                } break;
                case 4: {
                    const auto p1 {vec2{pos.x+(cw*0.5f+offset_x)*zoom_scale, pos.y+(ch*0.5f+offset_y)*zoom_scale}};
                    const auto p2 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y+(ch*0.5f+offset_y)*zoom_scale}};
                    const auto p3 {vec2{pos.x+(cw*0.5f+offset_x)*zoom_scale, pos.y-ch*0.5f*zoom_scale}};

                    resize_lines.emplace_back(line{p1, p2, {}, 1.0f});
                    resize_lines.emplace_back(line{p1, p3, {}, 1.0f});
                    if (offset_x > 0) {
                        const auto p4 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y-ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p4, p3, {}, 1.0f});
                    }
                    if (offset_y > 0) {
                        const auto p4 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y+ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p4, p2, {}, 1.0f});
                    }
                } break;
                case 5: {
                    const auto p1 {vec2{pos.x+(cw*0.5f+offset_x)*zoom_scale, pos.y+ch*0.5f*zoom_scale}};
                    const auto p2 {vec2{pos.x+(cw*0.5f+offset_x)*zoom_scale, pos.y-ch*0.5f*zoom_scale}};
                    resize_lines.emplace_back(line{p1, p2, {}, 1.0f});
                    if (offset_x > 0) {
                        const auto p3 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y+ch*0.5f*zoom_scale}};
                        const auto p4 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y-ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p1, p3, {}, 1.0f});
                        resize_lines.emplace_back(line{p2, p4, {}, 1.0f});
                    }
                } break;
                case 6: {
                    const auto p1 {vec2{pos.x+(cw*0.5f+offset_x)*zoom_scale, pos.y-(ch*0.5f+offset_y)*zoom_scale}};
                    const auto p2 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y-(ch*0.5f+offset_y)*zoom_scale}};
                    const auto p3 {vec2{pos.x+(cw*0.5f+offset_x)*zoom_scale, pos.y+ch*0.5f*zoom_scale}};

                    resize_lines.emplace_back(line{p1, p2, {}, 1.0f});
                    resize_lines.emplace_back(line{p1, p3, {}, 1.0f});
                    if (offset_x > 0) {
                        const auto p4 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y+ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p4, p3, {}, 1.0f});
                    }
                    if (offset_y > 0) {
                        const auto p4 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y-ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p4, p2, {}, 1.0f});
                    }
                } break;
                case 7: {
                    const auto p1 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y-(ch*0.5f+offset_y)*zoom_scale}};
                    const auto p2 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y-(ch*0.5f+offset_y)*zoom_scale}};
                    resize_lines.emplace_back(line{p1, p2, {}, 1.0f});
                    if (offset_y > 0) {
                        const auto p3 {vec2{pos.x-cw*0.5f*zoom_scale, pos.y-ch*0.5f*zoom_scale}};
                        const auto p4 {vec2{pos.x+cw*0.5f*zoom_scale, pos.y-ch*0.5f*zoom_scale}};
                        resize_lines.emplace_back(line{p3, p1, {}, 1.0f});
                        resize_lines.emplace_back(line{p4, p2, {}, 1.0f});
                    }
                } break;
                default:
                    break;
            }

            for (const auto& line:resize_lines) {
                for (const auto& l:generic_add_line(line)) {
                    line_batcher.lines_data.emplace_back(l);
                }
            }

            // using line batcher here as well. Think of a better way in the future.
            {
                int line_count {static_cast<int>((line_batcher.lines_data.size()/4))};
                int iterations {line_count / MAX_PER_BATCH};
                int leftover   {line_count - (MAX_PER_BATCH*iterations)};
                for (int i{}; i<iterations; ++i) {
                    graphics::buffer_upload_subdata(line_vbo, 0, 4*MAX_PER_BATCH*line_batcher::vertex_t::stride, line_batcher.lines_data.data()+i*MAX_PER_BATCH*4);
                    glDrawElements(GL_TRIANGLES, MAX_PER_BATCH*6, GL_UNSIGNED_INT, nullptr);
                }
                if (leftover > 0) {
                    graphics::buffer_upload_subdata(line_vbo, 0, 4*static_cast<u32>(leftover)*line_batcher::vertex_t::stride, line_batcher.lines_data.data()+iterations*MAX_PER_BATCH*4);
                    glDrawElements(GL_TRIANGLES, leftover*6, GL_UNSIGNED_INT, nullptr);
                }
                line_batcher.lines_data.clear();
            }
        }
    }

}

}
