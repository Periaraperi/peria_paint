#include "app.hpp"

#include <SDL3/SDL.h>
#include <chrono>
#include <filesystem>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <print>
#include <queue>
#include <utility>

#include "input_manager.hpp"
#include "graphics/graphics.hpp"

#ifdef PERIA_DEBUG
#include "graphics/gl_errors.hpp"
#endif

namespace {

constexpr int MAX_FPS {500};

[[nodiscard]]
float lerp(float a, float b, float t) noexcept
{ return a*(1.0f - t) + b*t; }

[[nodiscard]]
bool point_inside_rect(const peria::math::vec2f& point_to_check, const peria::math::vec2f& rect_lower_left, const peria::math::vec2f& rect_size) noexcept
{
    return point_to_check.x >= rect_lower_left.x && 
           point_to_check.x <= rect_lower_left.x+rect_size.x &&
           point_to_check.y >= rect_lower_left.y && 
           point_to_check.y <= rect_lower_left.y+rect_size.y;
}

// Generate Catmull-Rom spline which will go through each point in points based on parameter t
[[nodiscard]]
peria::math::vec2f get_point_on_path(const std::vector<peria::math::vec2f>& points, float t)
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

    return 0.5f * peria::math::vec2f{c1*points[p0].x + c2*points[p1].x + c3*points[p2].x + c4*points[p3].x,
                                     c1*points[p0].y + c2*points[p1].y + c3*points[p2].y + c4*points[p3].y};
}

[[nodiscard]]
std::vector<std::string> get_all_png_images()
{
    std::vector<std::string> res;
    std::filesystem::path p{"./saved/"};
    std::filesystem::directory_iterator it{p};
    for (const auto& entry:it) {
        if (entry.is_regular_file()) {
            const auto name {entry.path().filename().string()};
            if (name.size() > 4) { // only take [something].png files
                if (name.substr(name.size()-4) == ".png") res.emplace_back(name);
            }
        }
    }
    return res;
}

}

namespace peria {

namespace sdl {

sdl_initializer::sdl_initializer(const application_settings& settings) noexcept
{

#ifdef PERIA_DEBUG
    for (int i{}; i<SDL_GetNumVideoDrivers(); ++i) {
        if (std::string{SDL_GetVideoDriver(i)} == "x11") {
            SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "x11", SDL_HINT_OVERRIDE);
            break;
        }
    }
#endif
#ifndef PERIA_DEBUG
    for (int i{}; i<SDL_GetNumVideoDrivers(); ++i) {
        if (std::string{SDL_GetVideoDriver(i)} == "wayland") {
            SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland", SDL_HINT_OVERRIDE);
            break;
        }
    }
#endif

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
#ifdef PERIA_DEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG); // removing this will disable GL_CONTEXT_FLAG_DEBUG_BIT
#endif

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
{ return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow); };

} // end imgui

application::application(application_settings&& settings)
    :app_settings_{std::move(settings)},
     sdl_initializer_{app_settings_},
     imgui_{sdl_initializer_.window, sdl_initializer_.context, "#version 460"},
     circle_shader{"./assets/shaders/circle.vert", "./assets/shaders/circle.frag"},
     circle_batcher_shader{"./assets/shaders/circle_batcher.vert", "./assets/shaders/circle_batcher.frag"},
     colored_quad_shader{"./assets/shaders/quad_colored.vert", "./assets/shaders/quad_colored.frag"},
     textured_quad_shader{"./assets/shaders/quad.vert", "./assets/shaders/quad.frag"},
     sampler_linear{graphics::create_sampler(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER)}, 
     sampler_nearest{graphics::create_sampler(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER)}, 
     canvas_bg{gl::texture2d{graphics::create_texture2d_from_color(graphics::WHITE)}},
     canvas{gl::texture2d{graphics::create_texture2d(static_cast<int>(app_settings_.window_width*0.75f), 
                                                     static_cast<int>(app_settings_.window_height*0.75f),
                                                     GL_RGBA32F)},
            {}, 
            static_cast<int>(app_settings_.window_width*0.75f),
            static_cast<int>(app_settings_.window_height*0.75f),
            {},
            {}
     },
     temp_canvas{{gl::texture2d{graphics::create_texture2d(canvas.width, canvas.height, GL_RGBA32F)}}, {}}
{
    if (!sdl_initializer_.initialized) return;
    std::println("application construction");

    // initialize batchers
    {
        graphics::init_circle_batcher();
        graphics::init_quad_batcher();
    }

    input_manager::initialize();

    {
        peria::graphics::set_vsync(true);
        peria::graphics::set_relative_mouse(sdl_initializer_.window, false);
        peria::graphics::set_screen_size(app_settings_.window_width, app_settings_.window_height);
        window_projection = math::get_ortho_projection(0.0f, static_cast<float>(app_settings_.window_width), 0.0f, static_cast<float>(app_settings_.window_height));
    }

    // extensions and info
    {
        std::println("Vendor graphic card: {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        std::println("Renderer: {}",            reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        std::println("Version GL: {}",          reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        std::println("Version GLSL: {}",        reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

        int extension_count {};
        glGetIntegerv(GL_NUM_EXTENSIONS, &extension_count);
        std::println("Extension count = {}", extension_count);
        std::vector<std::string> extensions; extensions.reserve(static_cast<std::size_t>(extension_count));
        for (int i{}; i<extension_count; ++i) {
            // names of GL_EXTENSIONS are ascii strings. So using reinterpret cast is valid option
            std::string ext_name{reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<u32>(i)))};
            extensions.push_back(std::move(ext_name));
        }

        if (std::find(extensions.begin(), extensions.end(), "GL_ARB_debug_output") != extensions.end()) {
            std::println("{} is supported", "GL_ARB_debug_output");
        }
        else {
            std::println("{} is NOT supported", "GL_ARB_debug_output");
        }

#ifdef PERIA_DEBUG
        int gl_flags {};
        glGetIntegerv(GL_CONTEXT_FLAGS, &gl_flags);
        if (gl_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            std::println("Debug context flag is ON!");
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(gl::debug_callback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
        else {
            std::println("Debug context is not supported!");
        }
#endif
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
        canvas.pos = 0.5f*peria::math::vec2f{static_cast<float>(graphics::get_screen_size().w), static_cast<float>(graphics::get_screen_size().h)};
    }

    glEnable(GL_BLEND);

    // just clear canvas to default color before doing anything
    graphics::bind_frame_buffer(canvas.buffer);
    graphics::set_viewport(0, 0, canvas.width, canvas.height);
    const auto& [r, g, b] {info.bg_color};
    graphics::clear_buffer_color(canvas.buffer.id, graphics::color{r, g, b, 1.0f});

    stroke_history.strokes.emplace_back();
    if (!std::filesystem::exists(std::filesystem::path{"saved"})) {
        std::filesystem::create_directory(std::filesystem::path{"saved"});
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
                input_manager_->set_mouse_moved();
            }
            else if (ev.type == SDL_EVENT_MOUSE_WHEEL && !imgui_.is_imgui_hovered()) {
                input_manager_->set_mouse_wheel_motion(ev.wheel.y);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        update(dt);
        draw();
        //test();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::Button("save")) {
                info.current_filename = graphics::write_to_png(canvas.texture, canvas.width, canvas.height, info.current_filename.c_str());
            }

            if (ImGui::BeginMenu("open")) {
                const auto files {get_all_png_images()};
                for (const auto& f:files) {
                    if (ImGui::MenuItem(f.c_str())) {
                        auto image {graphics::load_png(("./saved/"+f).c_str())};

                        canvas.texture = std::move(image.texture);
                        canvas.width = image.width;
                        canvas.height = image.height;
                        canvas.projection = math::get_ortho_projection(0.0f, static_cast<float>(canvas.width), 0.0f, static_cast<float>(canvas.height));

                        glNamedFramebufferTexture(canvas.buffer.id, GL_COLOR_ATTACHMENT0, canvas.texture.id, 0);
                        auto status {glCheckNamedFramebufferStatus(canvas.buffer.id, GL_FRAMEBUFFER)};
                        if (status != GL_FRAMEBUFFER_COMPLETE) {
                            std::println("FrameBuffer with id {} is incomplete\n {}", canvas.buffer.id, status);
                        }

                        info.current_filename = f.substr(0, f.size()-4);
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::Button("tools")) {
                if (imgui_.show_tools) imgui_.show_tools = false;
                else                   imgui_.show_tools = true;
            }

            ImGui::Text("Width %d", canvas.width);
            ImGui::Text("Height %d", canvas.height);
            if (info.use_nearest) {
                ImGui::Text("Filtering - Nearest");
            }
            else {
                ImGui::Text("Filtering - Linear");
            }
            switch (current_mode) {
                case app_mode::DRAW:
                    if (current_brush_type == brush_type::PEN)
                        ImGui::Text("Mode - DRAW PEN");
                    if (current_brush_type == brush_type::BUCKET)
                        ImGui::Text("Mode - DRAW BUCKET");
                    break;
                case app_mode::ERASER:
                    ImGui::Text("Mode - ERASER");
                    break;
                case app_mode::RESIZE:
                    ImGui::Text("Mode - RESIZE");
                    break;
                default:
                    break;
            }

            ImGui::EndMainMenuBar();
        }

        if (imgui_.show_tools) {
            if (ImGui::Begin("tools")) {
                if (ImGui::ColorPicker3("brush_color", info.current_color.data())) {}
                if (ImGui::SliderFloat("brush_size", &info.current_brush_size, 1.0f, static_cast<float>(std::min(canvas.width, canvas.height))/4.0f)) {}
                if (ImGui::Button("center")) {
                    cam2d.pos = {};
                    cam2d.zoom_scale = 1.0f;
                }
                if (ImGui::Button("pen")) {
                    current_mode = app_mode::DRAW;
                    current_brush_type = brush_type::PEN;
                }
                if (ImGui::Button("eraser")) {
                    current_mode = app_mode::ERASER;
                    current_brush_type = brush_type::ERASER;
                }
                if (ImGui::Button("bucket")) {
                    current_mode = app_mode::DRAW;
                    current_brush_type = brush_type::BUCKET;
                }
                if (ImGui::Button("toggle filtering")) {
                    info.use_nearest = !info.use_nearest;
                }
            }
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(sdl_initializer_.window);

        {
            std::string title {"peria_paint | "+std::to_string(elapsed_time)+" "+std::to_string(dt)};
            if (graphics::is_vsync()) title += " VSYNC: ON";
            else title += " VSYNC: OFF";

            SDL_SetWindowTitle(sdl_initializer_.window, title.c_str());
        }

        input_manager_->update_prev_state();
    }
}

void application::update([[maybe_unused]]float dt)
{
    const auto im {input_manager::instance()};
    if (im->key_pressed(SDL_SCANCODE_B)) {
        current_mode = app_mode::DRAW;
        current_brush_type = brush_type::PEN;
    }
    if (im->key_pressed(SDL_SCANCODE_R)) {
        current_mode = app_mode::RESIZE;
    }
    if (im->key_pressed(SDL_SCANCODE_E)) {
        current_mode = app_mode::ERASER;
        current_brush_type = brush_type::ERASER;
    }
    if (im->key_pressed(SDL_SCANCODE_G)) {
        current_mode = app_mode::DRAW;
        current_brush_type = brush_type::BUCKET;
    }
    if (im->key_down(SDL_SCANCODE_LCTRL) && im->key_pressed(SDL_SCANCODE_Z)) {
        stroke_history.should_undo = true;
    }
    if (im->key_down(SDL_SCANCODE_LCTRL) && im->key_pressed(SDL_SCANCODE_Y)) {
        stroke_history.should_redo = true;
    }
    if (im->key_down(SDL_SCANCODE_LCTRL) && im->key_pressed(SDL_SCANCODE_S)) {
        info.current_filename = graphics::write_to_png(canvas.texture, canvas.width, canvas.height, info.current_filename.c_str());
    }

    cam2d.update(window_projection);

    const math::vec2f mouse_screen {im->get_mouse_gl().x, im->get_mouse_gl().y};
    const math::vec2f mouse_world {cam2d.screen_to_world(mouse_screen, window_projection)};
    const math::vec2f canvas_world_lower_left {canvas.pos-math::vec2{canvas.width*0.5f, canvas.height*0.5f}};
    const bool inside_canvas {point_inside_rect(mouse_world, canvas_world_lower_left, {static_cast<float>(canvas.width), static_cast<float>(canvas.height)})};

    switch (current_mode) {
        case app_mode::DRAW:
        case app_mode::ERASER:
        {
            if (current_brush_type == brush_type::BUCKET && inside_canvas && im->mouse_pressed(mouse_button::LEFT) && !imgui_.is_imgui_hovered()) {
                glPixelStorei(GL_UNPACK_ROW_LENGTH, canvas.width);
                std::vector<float> pixels(static_cast<std::size_t>(canvas.width*canvas.height*4), 0);
                glGetTextureImage(canvas.texture.id, 0, GL_RGBA, GL_FLOAT, static_cast<int>(pixels.size()*sizeof(float)), &pixels[0]);

                // mouse position relative to canvas cast to ints to get pixel coords. (lower left of canvas is zero zero)
                const math::vec2i mp {static_cast<int>(mouse_world.x-canvas_world_lower_left.x), static_cast<int>(mouse_world.y-canvas_world_lower_left.y)};

                std::vector<bool> visited(static_cast<std::size_t>(canvas.width)*static_cast<std::size_t>(canvas.height), false);

                std::queue<math::vec2i> q;
                q.push(mp);

                std::vector<math::vec2i> to_color; 
                to_color.reserve(visited.size()/2); // approx space

                // reuse brush color for bucket tool
                math::vec3f new_color {info.current_color[0], info.current_color[1], info.current_color[2]};

                math::vec3f old_color {
                    pixels[static_cast<std::size_t>((mp.y*canvas.width+mp.x)*4 + 0)],
                    pixels[static_cast<std::size_t>((mp.y*canvas.width+mp.x)*4 + 1)],
                    pixels[static_cast<std::size_t>((mp.y*canvas.width+mp.x)*4 + 2)]
                };

                auto float_eq = [](float a, float b) {
                    constexpr float epsilon {0.000001f};
                    return std::abs(a-b) <= (std::max(a, b)*epsilon);
                };

                while (!q.empty()) {
                    const auto coord {q.front()}; q.pop();
                    if (visited[static_cast<std::size_t>(coord.y*canvas.width+coord.x)]) {
                        continue;
                    }
                    visited[static_cast<std::size_t>(coord.y*canvas.width+coord.x)] = true;
                    to_color.emplace_back(coord.x, coord.y);
                    for (int dx{-1}; dx<=1; ++dx) {
                        for (int dy{-1}; dy<=1; ++dy) {
                            if (dx == 0 && dy == 0) continue;
                            const auto x {coord.x + dx};
                            const auto y {coord.y + dy};
                            if ((x>=0 && x<canvas.width && y>=0 && y<canvas.height) && 
                                !visited[static_cast<std::size_t>(y*canvas.width+x)] &&
                                float_eq(pixels[static_cast<std::size_t>((y*canvas.width+x)*4 + 0)], old_color.x) &&
                                float_eq(pixels[static_cast<std::size_t>((y*canvas.width+x)*4 + 1)], old_color.y) &&
                                float_eq(pixels[static_cast<std::size_t>((y*canvas.width+x)*4 + 2)], old_color.z)) {
                                q.push({x, y});
                            }
                        }
                    }
                }
                for (const auto& [x, y]:to_color) {
                    pixels[static_cast<std::size_t>((y*canvas.width+x)*4 + 0)] = new_color.x; 
                    pixels[static_cast<std::size_t>((y*canvas.width+x)*4 + 1)] = new_color.y;
                    pixels[static_cast<std::size_t>((y*canvas.width+x)*4 + 2)] = new_color.z;
                    pixels[static_cast<std::size_t>((y*canvas.width+x)*4 + 3)] = 1.0f;
                }

                glTextureSubImage2D(canvas.texture.id, 0, 
                                    0, 0,
                                    canvas.width, canvas.height,
                                    GL_RGBA, GL_FLOAT, &pixels[0]);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // don't forget this
                return;
            }
            else if (current_brush_type == brush_type::BUCKET) return;

            if ((info.should_start_new_stroke && !inside_canvas) ||
                (info.should_start_new_stroke && (imgui_.is_imgui_captured() || imgui_.is_imgui_hovered()))) break;
            
            auto& strokes {stroke_history.strokes};
            auto& last_stroke_index {stroke_history.last_stroke_index};
            auto& last_valid_redo_index {stroke_history.last_valid_redo_index};

            if (im->mouse_moving()) {
                if (im->mouse_down(mouse_button::LEFT)) {
                    if (info.should_start_new_stroke) {
                        ++last_stroke_index;
                        if (stroke_history.last_stroke_index >= strokes.size()) strokes.emplace_back();
                        for (std::size_t i{last_stroke_index}; i<=last_valid_redo_index; ++i) {
                            strokes[i].brush_size = 0.0f;
                            strokes[i].aa = 0.0f;
                            strokes[i].brush_points.clear();
                            strokes[i].type = current_brush_type;
                        }
                        last_valid_redo_index = last_stroke_index;
                    }
                    strokes[last_stroke_index].brush_points.emplace_back(math::vec2f{mouse_world-canvas_world_lower_left});
                    strokes[last_stroke_index].brush_size = info.current_brush_size;
                    strokes[last_stroke_index].aa = info.current_aa;
                    strokes[last_stroke_index].color = math::vec3f{info.current_color[0], info.current_color[1], info.current_color[2]};
                    strokes[last_stroke_index].type = current_brush_type;
                    info.drawing = true;
                    info.drawing_finished = false;
                    info.should_start_new_stroke = false;
                }
                if (im->mouse_released(mouse_button::LEFT)) {
                    strokes[last_stroke_index].brush_points.emplace_back(math::vec2f{mouse_world-canvas_world_lower_left});
                    strokes[last_stroke_index].brush_size = info.current_brush_size;
                    strokes[last_stroke_index].aa = info.current_aa;
                    strokes[last_stroke_index].color = math::vec3f{info.current_color[0], info.current_color[1], info.current_color[2]};
                    strokes[last_stroke_index].type = current_brush_type;
                    info.drawing = true;
                    info.drawing_finished = true;
                    info.should_start_new_stroke = true;
                }
            }
            else {
                if (im->mouse_released(mouse_button::LEFT)) {
                    if (info.should_start_new_stroke) {
                        ++last_stroke_index;
                        if (last_stroke_index >= strokes.size()) strokes.emplace_back();
                        for (std::size_t i{last_stroke_index}; i<=last_valid_redo_index; ++i) {
                            strokes[i].brush_size = 0.0f;
                            strokes[i].aa = 0.0f;
                            strokes[i].brush_points.clear();
                        }
                        strokes[last_stroke_index].brush_points.emplace_back(math::vec2f{mouse_world-canvas_world_lower_left});
                        strokes[last_stroke_index].brush_size = info.current_brush_size;
                        strokes[last_stroke_index].aa = info.current_aa;
                        strokes[last_stroke_index].color = math::vec3f{info.current_color[0], info.current_color[1], info.current_color[2]};
                        strokes[last_stroke_index].type = current_brush_type;
                        last_valid_redo_index = last_stroke_index;
                    }
                    info.drawing = true;
                    info.drawing_finished = true;
                    info.should_start_new_stroke = true;
                }
            }    
        } 
            break;
        case app_mode::RESIZE: 
        {
            if (!info.resizing) {
                // determine if we started resizing, and from which button.
                int index {-1};
                int k {};
                for (const auto& [dx, dy]:canvas_resize_dirs) {
                    const math::vec2f button_lower_left {
                        canvas.pos.x+(0.5f*static_cast<float>(canvas.width*dx))-info.resize_button_radius,
                        canvas.pos.y+(0.5f*static_cast<float>(canvas.height*dy))-info.resize_button_radius,
                    };
                    if (point_inside_rect(mouse_world, button_lower_left, {2.0f*info.resize_button_radius, 2.0f*info.resize_button_radius})) {
                        index = k;
                        break;
                    }
                    ++k;
                }
                if(index != -1 && im->mouse_pressed(mouse_button::LEFT)) {
                    info.resizing = true;
                    info.resize_button_index = index;
                }
            }
            if (info.resizing && im->mouse_down(mouse_button::LEFT)) {
                const auto resize_dir {canvas_resize_dirs[static_cast<std::size_t>(info.resize_button_index)]};
                const auto dx {mouse_world.x-(canvas.pos.x+resize_dir.x*canvas.width*0.5f)};
                const auto dy {mouse_world.y-(canvas.pos.y+resize_dir.y*canvas.height*0.5f)};
                info.new_width  = static_cast<int>(static_cast<float>(canvas.width )+dx*static_cast<float>(resize_dir.x));
                info.new_height = static_cast<int>(static_cast<float>(canvas.height)+dy*static_cast<float>(resize_dir.y));
            }
            if (info.resizing && im->mouse_released(mouse_button::LEFT)) {
                temp_canvas.texture = graphics::create_texture2d(info.new_width, info.new_height, GL_RGBA32F);
                glNamedFramebufferTexture(temp_canvas.buffer.id, GL_COLOR_ATTACHMENT0, temp_canvas.texture.id, 0);
                const auto status {glCheckNamedFramebufferStatus(temp_canvas.buffer.id, GL_FRAMEBUFFER)};
                if (status != GL_FRAMEBUFFER_COMPLETE) {
                    std::println("FrameBuffer with id {} is incomplete\n {}", temp_canvas.buffer.id, status);
                }
                info.resizing = false;
                info.resized = true;
            }
        }
            break;
        default:
            break;
    }
}

void application::draw()
{
    //ImGui::Text("%zu", stroke_history.strokes[stroke_history.last_stroke_index].brush_points.size());
    //ImGui::Text("last stroke idx %zu", stroke_history.last_stroke_index);
    //ImGui::Text("last valid redo idx %zu", stroke_history.last_valid_redo_index);
    //ImGui::Text("size %zu", stroke_history.strokes.size());
    //ImGui::Text("canvas dims %d, %d", canvas.width, canvas.height);

    // Clear resized buffer and copy all contents from old to new.
    // Then swap struct info as well.
    if (info.resized) {
        temp_canvas.width  = info.new_width;
        temp_canvas.height = info.new_height;
        graphics::bind_frame_buffer(temp_canvas.buffer);
        graphics::set_viewport(0, 0, temp_canvas.width, temp_canvas.height);
        const auto& [r, g, b] {info.bg_color};
        graphics::clear_buffer_color(temp_canvas.buffer.id, graphics::color{r, g, b, 1.0f});

        const auto w {std::min(temp_canvas.width,  canvas.width)};
        const auto h {std::min(temp_canvas.height, canvas.height)};

        math::vec2f dst {};
        math::vec2f src {};
        float dw {static_cast<float>(info.new_width-canvas.width)};
        float dh {static_cast<float>(info.new_height-canvas.height)};
        math::vec2f canvas_old_lower_left {canvas.pos-math::vec2f{canvas.width*0.5f, canvas.height*0.5f}};
        switch (info.resize_button_index) {
            case 0:
                if (dw > 0) dst.x =  dw;
                else        src.x = -dw;
                if (dh > 0) dst.y =  dh;
                else        src.y = -dh;
                canvas.pos += math::vec2f{-dw*0.5f, -dh*0.5f};
                break;
            case 1: 
            case 2:
                if (dw > 0) dst.x =  dw;
                else        src.x = -dw;
                canvas.pos += math::vec2f{-dw*0.5f, dh*0.5f};
                break;
            case 3:
            case 4:
            case 5:
                canvas.pos += math::vec2f{dw*0.5f, dh*0.5f};
                break;
            case 6: 
            case 7:
                if (dh > 0) dst.y =  dh;
                else        src.y = -dh;
                canvas.pos += math::vec2{dw*0.5f, -dh*0.5f};
                break;
            default:
                break;
        }

        glCopyImageSubData(canvas.texture.id, GL_TEXTURE_2D , 0, static_cast<int>(src.x), static_cast<int>(src.y), 0,
                           temp_canvas.texture.id, GL_TEXTURE_2D, 0, static_cast<int>(dst.x), static_cast<int>(dst.y), 0, 
                           w, h, 1);

        std::swap(temp_canvas.buffer.id, canvas.buffer.id);
        std::swap(temp_canvas.texture.id, canvas.texture.id);
        std::swap(temp_canvas.width, canvas.width);
        std::swap(temp_canvas.height, canvas.height);

        math::vec2f canvas_new_lower_left {canvas.pos-math::vec2f{canvas.width*0.5f, canvas.height*0.5f}};
        canvas.projection = math::get_ortho_projection(0.0f, static_cast<float>(canvas.width), 0.0f, static_cast<float>(canvas.height));

        info.resized = false;
        info.resize_button_index = -1;

        // offset brush control points so that undo/redo doesn't break after resizing.
        for (auto& s:stroke_history.strokes) {
            for (auto& p:s.brush_points) {
                p += (canvas_old_lower_left - canvas_new_lower_left);
            }
        }
    }


    // Helper lambdas for repetitive rendering

    auto render_single_point = [this](const stroke& stroke) {
        // Special case where we don't move mouse and only draw a point.
        // No interpolation needed. Just draw single brush point.
        graphics::bind_vertex_array(circle_vao);

        const auto& p {stroke.brush_points.front()};
        math::mat4f m {math::translate(p.x, p.y, 0.0f)*
                       math::scale(2.0f*stroke.brush_size, 2.0f*stroke.brush_size, 1.0f)};

        circle_shader.set_int("u_is_ring", 0);
        circle_shader.set_mat4("u_vp", canvas.projection);
        circle_shader.set_mat4("u_model", m);
        circle_shader.set_float("u_radius", stroke.brush_size);
        circle_shader.set_vec2("u_center", p);
        circle_shader.set_vec3("u_color", stroke.color);
        circle_shader.set_float("u_aa", stroke.aa);
        circle_shader.use_shader();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    };

    auto render_points_with_line = [this](const stroke& stroke) {
        // Handle special case where stroke control points have more than a single point
        // and less than the amount needed for spline interpolation.
        // Draw a single line through them.

        circle_shader.use_shader();
        circle_shader.set_int("u_is_ring", 0);
        const auto& first {stroke.brush_points.front()};
        const auto& last  {stroke.brush_points.back()};

        for (const auto& [x, y]:{first, last}) {
            graphics::bind_vertex_array(circle_vao);
            math::mat4f m {math::translate(x, y, 0.0f)*
                           math::scale(2.0f*stroke.brush_size, 2.0f*stroke.brush_size, 1.0f)};

            circle_shader.set_mat4("u_vp", canvas.projection);
            circle_shader.set_mat4("u_model", m);
            circle_shader.set_float("u_radius", stroke.brush_size);
            circle_shader.set_vec2("u_center", {x, y});
            circle_shader.set_vec3("u_color", stroke.color);
            circle_shader.set_float("u_aa", stroke.aa);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        colored_quad_shader.set_mat4("u_mvp", canvas.projection);
        graphics::draw_lines({graphics::line{first, last, stroke.brush_size, stroke.color}}, colored_quad_shader);
    };

    auto render_stroke = [this](const stroke& stroke) {
        std::vector<graphics::circle> samples;
        for (float t{}; t<static_cast<float>(stroke.brush_points.size())-3.0f; t+=0.01f) {
            samples.emplace_back(graphics::circle{get_point_on_path(stroke.brush_points, t), stroke.color, stroke.brush_size});
        }

        circle_batcher_shader.set_mat4("u_mvp", canvas.projection);
        circle_batcher_shader.set_float("u_aa", stroke.aa);
        graphics::draw_circles(samples, circle_batcher_shader);
    };

    if (stroke_history.should_undo) {
        graphics::bind_frame_buffer(canvas.buffer);
        graphics::set_viewport(0, 0, canvas.width, canvas.height);
        const auto& [r, g, b] {info.bg_color};
        graphics::clear_buffer_color(canvas.buffer.id, graphics::color{r, g, b, 1.0f});

        if (stroke_history.last_stroke_index > 0) {
            --stroke_history.last_stroke_index;
        }

        for (std::size_t i{1}; i<=stroke_history.last_stroke_index; ++i) {
            const auto& stroke {stroke_history.strokes[i]};

            if (stroke.type == brush_type::PEN) {
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            }
            if (stroke.type == brush_type::ERASER) {
                glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
            }

            if (stroke.brush_points.size() == 1) {
                render_single_point(stroke);
            }
            else if (stroke.brush_points.size() == 2 || stroke.brush_points.size() == 3) {
                render_points_with_line(stroke);
            }
            else if (stroke.brush_points.size() >= 4) {
                render_stroke(stroke);
            }
        }
        stroke_history.should_undo = false;
    }
    else if (stroke_history.should_redo) {
        if (stroke_history.last_stroke_index+1 <= stroke_history.last_valid_redo_index) {
            graphics::bind_frame_buffer(canvas.buffer);
            graphics::set_viewport(0, 0, canvas.width, canvas.height);

            ++stroke_history.last_stroke_index;
            const auto& stroke {stroke_history.strokes[stroke_history.last_stroke_index]};

            if (stroke.type == brush_type::PEN) {
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            }
            if (stroke.type == brush_type::ERASER) {
                glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
            }

            if (stroke.brush_points.size() == 1) {
                render_single_point(stroke);
            }
            else if (stroke.brush_points.size() == 2 || stroke.brush_points.size() == 3) {
                render_points_with_line(stroke);
            }
            else if (stroke.brush_points.size() >= 4) {
                render_stroke(stroke);
            }
        }
        stroke_history.should_redo = false;
    }

    if (info.drawing) {

        if (current_mode == app_mode::DRAW) {
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
        if (current_mode == app_mode::ERASER) {
            glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
        }

        graphics::bind_frame_buffer(canvas.buffer);
        graphics::set_viewport(0, 0, canvas.width, canvas.height);

        const auto& stroke {stroke_history.strokes[stroke_history.last_stroke_index]};
        if (stroke.brush_points.size() == 1 && info.drawing_finished) {
            render_single_point(stroke);
        }
        else if ((stroke.brush_points.size() == 2 || stroke.brush_points.size() == 3) && info.drawing_finished) {
            render_points_with_line(stroke);
        }
        else if (stroke.brush_points.size() >= 4 && input_manager::instance()->mouse_moving()) {
            std::vector<math::vec2f> ps;
            for (std::size_t i{stroke.brush_points.size()-4}; i<stroke.brush_points.size(); ++i) {
                ps.emplace_back(stroke.brush_points[i]);
            }
            render_stroke({std::move(ps), stroke.aa, stroke.brush_size, stroke.color, stroke.type});
        }

        if (info.drawing_finished) {
            info.drawing_finished = false;
            info.drawing = false;
            info.should_start_new_stroke = true;
        }
    }

    // Everything below is done in main framebuffer
    // POST canvas rendering / final pass
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        graphics::set_viewport(0, 0, app_settings_.window_width, app_settings_.window_height);
        graphics::bind_frame_buffer_default();
        graphics::clear_buffer_all(0, graphics::GREY, 1.0f, 0);

        math::mat4f canvas_world_model {math::translate(canvas.pos.x, canvas.pos.y, 0.0f)*
                                        math::scale(canvas.width, canvas.height, 1.0f)};

        graphics::bind_vertex_array(canvas_vao);
        textured_quad_shader.use_shader();
        textured_quad_shader.set_mat4("u_mvp", window_projection*cam2d.view*canvas_world_model);

        //graphics::bind_texture_and_sampler(canvas_bg, canvas.sampler, 0);
        //const auto& [r, g, b] {info.bg_color};
        //textured_quad_shader.set_vec3("u_color_multiplier", {r, g, b});
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        if (info.use_nearest) graphics::bind_texture_and_sampler(canvas.texture, sampler_nearest, 0);
        else graphics::bind_texture_and_sampler(canvas.texture, sampler_linear, 0);
        textured_quad_shader.set_vec3("u_color_multiplier", {1.0f, 1.0f, 1.0f});
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        if (current_mode == app_mode::RESIZE) {
            graphics::bind_vertex_array(circle_vao);

            for (const auto& [dx, dy]:canvas_resize_dirs) {
                auto pos {canvas.pos+math::vec2f{0.5f*canvas.width*dx, 0.5f*canvas.height*dy}};
                math::mat4f m {math::translate(pos.x, pos.y, 0.0f)*
                               math::scale(2.0f*info.resize_button_radius, 2.0f*info.resize_button_radius, 1.0f)};

                circle_shader.use_shader();
                circle_shader.set_int("u_is_ring", 0);
                circle_shader.set_mat4("u_vp", window_projection*cam2d.view);
                circle_shader.set_mat4("u_model", m);
                circle_shader.set_float("u_radius", info.resize_button_radius);
                circle_shader.set_vec2("u_center", pos);
                circle_shader.set_vec3("u_color", math::vec3f{0.0f, 0.0f, 0.0f});
                circle_shader.set_float("u_aa", 0.0f);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            }

            if (info.resizing) {
                const auto offset_x {info.new_width -canvas.width };
                const auto offset_y {info.new_height-canvas.height};
                const auto& cw {canvas.width};
                const auto& ch {canvas.height};
                std::vector<graphics::line> resize_lines;
                switch (info.resize_button_index) {
                    case 0: {
                        const auto p1 {math::vec2f{canvas.pos.x-(cw*0.5f+offset_x), canvas.pos.y-(ch*0.5f+offset_y)}};
                        const auto p2 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y-(ch*0.5f+offset_y)}};
                        const auto p3 {math::vec2f{canvas.pos.x-(cw*0.5f+offset_x), canvas.pos.y+ch*0.5f}};

                        resize_lines.emplace_back(graphics::line{p1, p2, 1.0f});
                        resize_lines.emplace_back(graphics::line{p1, p3, 1.0f});
                        if (offset_x > 0) {
                            const auto p4 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y+ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p4, p3, 1.0f});
                        }
                        if (offset_y > 0) {
                            const auto p4 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y-ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p4, p2, 1.0f});
                        }
                    } break;
                    case 1: {
                        const auto p1 {math::vec2f{canvas.pos.x-(cw*0.5f+offset_x), canvas.pos.y+ch*0.5f}};
                        const auto p2 {math::vec2f{canvas.pos.x-(cw*0.5f+offset_x), canvas.pos.y-ch*0.5f}};
                        resize_lines.emplace_back(graphics::line{p1, p2, 1.0f});
                        if (offset_x > 0) {
                            const auto p3 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y+ch*0.5f}};
                            const auto p4 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y-ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p1, p3, 1.0f});
                            resize_lines.emplace_back(graphics::line{p2, p4, 1.0f});
                        }
                    } break;
                    case 2: {
                        const auto p1 {math::vec2f{canvas.pos.x-(cw*0.5f+offset_x), canvas.pos.y+(ch*0.5f+offset_y)}};
                        const auto p2 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y+(ch*0.5f+offset_y)}};
                        const auto p3 {math::vec2f{canvas.pos.x-(cw*0.5f+offset_x), canvas.pos.y-ch*0.5f}};

                        resize_lines.emplace_back(graphics::line{p1, p2, 1.0f});
                        resize_lines.emplace_back(graphics::line{p1, p3, 1.0f});
                        if (offset_x > 0) {
                            const auto p4 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y-ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p4, p3, 1.0f});
                        }
                        if (offset_y > 0) {
                            const auto p4 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y+ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p4, p2, 1.0f});
                        }
                    } break;
                    case 3: {
                        const auto p1 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y+(ch*0.5f+offset_y)}};
                        const auto p2 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y+(ch*0.5f+offset_y)}};
                        resize_lines.emplace_back(graphics::line{p1, p2, 1.0f});
                        if (offset_y > 0) {
                            const auto p3 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y+ch*0.5f}};
                            const auto p4 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y+ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p3, p1, 1.0f});
                            resize_lines.emplace_back(graphics::line{p4, p2, 1.0f});
                        }
                    } break;
                    case 4: {
                        const auto p1 {math::vec2f{canvas.pos.x+(cw*0.5f+offset_x), canvas.pos.y+(ch*0.5f+offset_y)}};
                        const auto p2 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y+(ch*0.5f+offset_y)}};
                        const auto p3 {math::vec2f{canvas.pos.x+(cw*0.5f+offset_x), canvas.pos.y-ch*0.5f}};

                        resize_lines.emplace_back(graphics::line{p1, p2, 1.0f});
                        resize_lines.emplace_back(graphics::line{p1, p3, 1.0f});
                        if (offset_x > 0) {
                            const auto p4 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y-ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p4, p3, 1.0f});
                        }
                        if (offset_y > 0) {
                            const auto p4 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y+ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p4, p2, 1.0f});
                        }
                    } break;
                    case 5: {
                        const auto p1 {math::vec2f{canvas.pos.x+(cw*0.5f+offset_x), canvas.pos.y+ch*0.5f}};
                        const auto p2 {math::vec2f{canvas.pos.x+(cw*0.5f+offset_x), canvas.pos.y-ch*0.5f}};
                        resize_lines.emplace_back(graphics::line{p1, p2, 1.0f});
                        if (offset_x > 0) {
                            const auto p3 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y+ch*0.5f}};
                            const auto p4 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y-ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p1, p3, 1.0f});
                            resize_lines.emplace_back(graphics::line{p2, p4, 1.0f});
                        }
                    } break;
                    case 6: {
                        const auto p1 {math::vec2f{canvas.pos.x+(cw*0.5f+offset_x), canvas.pos.y-(ch*0.5f+offset_y)}};
                        const auto p2 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y-(ch*0.5f+offset_y)}};
                        const auto p3 {math::vec2f{canvas.pos.x+(cw*0.5f+offset_x), canvas.pos.y+ch*0.5f}};

                        resize_lines.emplace_back(graphics::line{p1, p2, 1.0f});
                        resize_lines.emplace_back(graphics::line{p1, p3, 1.0f});
                        if (offset_x > 0) {
                            const auto p4 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y+ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p4, p3, 1.0f});
                        }
                        if (offset_y > 0) {
                            const auto p4 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y-ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p4, p2, 1.0f});
                        }
                    } break;
                    case 7: {
                        const auto p1 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y-(ch*0.5f+offset_y)}};
                        const auto p2 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y-(ch*0.5f+offset_y)}};
                        resize_lines.emplace_back(graphics::line{p1, p2, 1.0f});
                        if (offset_y > 0) {
                            const auto p3 {math::vec2f{canvas.pos.x-cw*0.5f, canvas.pos.y-ch*0.5f}};
                            const auto p4 {math::vec2f{canvas.pos.x+cw*0.5f, canvas.pos.y-ch*0.5f}};
                            resize_lines.emplace_back(graphics::line{p3, p1, 1.0f});
                            resize_lines.emplace_back(graphics::line{p4, p2, 1.0f});
                        }
                    } break;
                    default:
                        break;
                }

                colored_quad_shader.set_mat4("u_mvp", window_projection*cam2d.view);
                graphics::draw_lines(resize_lines, colored_quad_shader);
            }
        }
    }
}

std::vector<std::vector<math::vec2f>> strokes;
void application::test()
{
    const auto im {input_manager::instance()};
    cam2d.update(window_projection);

    const math::vec2f mouse_screen {im->get_mouse_gl().x, im->get_mouse_gl().y};
    const math::vec2f mouse_world {cam2d.screen_to_world(mouse_screen, window_projection)};
    if (strokes.empty()) strokes.emplace_back();


    if (im->mouse_moving()) {
        if (im->mouse_down(mouse_button::LEFT)) {
            strokes.back().emplace_back(mouse_world);
        }
        if (im->mouse_released(mouse_button::LEFT)) {
            strokes.back().emplace_back(mouse_world);
            strokes.emplace_back();
        }
    }

    {
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        graphics::set_viewport(0, 0, app_settings_.window_width, app_settings_.window_height);
        graphics::bind_frame_buffer_default();
        graphics::clear_buffer_all(0, graphics::WHITE, 1.0f, 0);

        circle_batcher_shader.set_mat4("u_mvp", window_projection*cam2d.view);

        std::vector<graphics::circle> circles;
        for (const auto& points:strokes) {
            for (float t{}; t<static_cast<float>(points.size())-3.0f; t+=0.01f) {
                circles.emplace_back(graphics::circle{get_point_on_path(points, t), {1.0f, 0.2f, 0.5f}, 30});
            }
        }
        graphics::draw_circles(circles, circle_batcher_shader);

        //graphics::bind_vertex_array(circle_vao);
        //math::mat4f m {math::translate(mouse_world.x, mouse_world.y, 0.0f)*
        //               math::scale(30.0f, 30.0f, 1.0f)};

        //circle_shader.set_int("u_is_ring", 0);
        //circle_shader.set_mat4("u_vp", window_projection*cam2d.view);
        //circle_shader.set_mat4("u_model", m);
        //circle_shader.set_float("u_radius", 15.0f);
        //circle_shader.set_vec2("u_center", mouse_world);
        //circle_shader.set_vec3("u_color", {1.0f, 0.5f, 1.0f});
        //circle_shader.set_float("u_aa", 1);
        //circle_shader.use_shader();
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    }
}

}
