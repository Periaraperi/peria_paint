#include "app.hpp"

#include <SDL3/SDL.h>
#include <chrono>
#include <glad/glad.h>

#include <print>
#include <utility>

#include "input_manager.hpp"
#include "graphics/graphics.hpp"

namespace {

constexpr int MAX_FPS {500};

[[nodiscard]]
float lerp(float a, float b, float t) noexcept
{ return a*(1.0f - t) + b*t; }

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

application::application(application_settings&& settings)
    :app_settings_{std::move(settings)},
     sdl_initializer_{app_settings_},
     circle_shader{"./assets/shaders/circle.vert", "./assets/shaders/circle.frag"},
     textured_quad_shader{"./assets/shaders/quad.vert", "./assets/shaders/quad.frag"},
     canvas{gl::texture2d{graphics::create_texture2d(400, 300, GL_RGBA8)},
            gl::sampler{graphics::create_sampler(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER)}, {}, 
            400, 300, {}, {}, {}, graphics::WHITE},
     temp_canvas{{gl::texture2d{graphics::create_texture2d(400, 300, GL_RGBA8)}}, {}, 400, 300},
     line_shader{"./assets/shaders/line.vert", "./assets/shaders/line.frag"}
{
    if (!sdl_initializer_.initialized) return;
    std::println("application construction");

    input_manager::initialize();

    {
        //TODO: come back to these settings later
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);

        peria::graphics::set_vsync(true);
        peria::graphics::set_relative_mouse(sdl_initializer_.window, false);
        peria::graphics::set_screen_size(app_settings_.window_width, app_settings_.window_height);
        window_projection = math::get_ortho_projection(0.0f, static_cast<float>(app_settings_.window_width), 0.0f, static_cast<float>(app_settings_.window_height));
    }
    // GL entities here
    {
        std::array<u32, 6> indices {0,1,2, 0,2,3};

        std::array<gl::vertex<gl::pos2, gl::pos2, gl::color4, gl::attr<float, 1>>, 4> circle_data {{
            {{-0.5f, -0.5f}, {0, 0}, {0.0f, 0.1f, 0.1f, 1.0f}, {0}},
            {{-0.5f,  0.5f}, {0, 0}, {0.0f, 0.1f, 0.1f, 1.0f}, {0}},
            {{ 0.5f,  0.5f}, {0, 0}, {0.0f, 0.1f, 0.1f, 1.0f}, {0}},
            {{ 0.5f, -0.5f}, {0, 0}, {0.0f, 0.1f, 0.1f, 1.0f}, {0}},
        }};

        graphics::buffer_upload_data(vbo, circle_data, GL_STATIC_DRAW);
        graphics::buffer_upload_data(ibo, indices, GL_STATIC_DRAW);

        graphics::vao_configure<gl::pos2, gl::pos2, gl::color4, gl::attr<float, 1>>(vao, vbo, 0);
        graphics::vao_connect_ibo(vao, ibo);

        std::array<gl::vertex<gl::pos2, gl::texture_coord>, 4> canvas_data {{
            {{-0.5f, -0.5f}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f}, {0.0f, 1.0f}},
            {{ 0.5f,  0.5f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f}, {1.0f, 0.0f}},
        }};

        graphics::buffer_upload_data(canvas_vbo, canvas_data, GL_STATIC_DRAW);

        graphics::vao_configure<gl::pos2, gl::texture_coord>(canvas_vao, canvas_vbo, 0);
        graphics::vao_connect_ibo(canvas_vao, ibo);

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
        canvas.pos_x = 0.5f*(static_cast<float>(graphics::get_screen_size().w));
        canvas.pos_y = 0.5f*(static_cast<float>(graphics::get_screen_size().h));

        // TEST STUFF
        {
        }
    }

    cpx.reserve(1024);
    cpy.reserve(1024);
    cpr.reserve(1024);
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
        }

        update(dt);
        
        if (0) {
            draw();
        }
        else {
            test_draw();
        }

        SDL_GL_SwapWindow(sdl_initializer_.window);
        info.mouse_moved = false;
        
        std::string title {"peria_paint | "+std::to_string(elapsed_time)+" "+std::to_string(dt)};
        if (graphics::is_vsync()) title += " VSYNC: ON";
        else title += " VSYNC: OFF";

        SDL_SetWindowTitle(sdl_initializer_.window, title.c_str());

        input_manager_->update_prev_state();
    }
}

void application::update(float dt)
{
    const auto im {input_manager::instance()};
    const auto [mx, my] {im->get_mouse_gl()};
    info.should_draw = false;

    if (im->key_down(SDL_SCANCODE_W)) {
        info.brush_size += 10.0f*dt;
        info.brush_size = std::min(info.brush_size, 150.0f);
    }
    if (im->key_down(SDL_SCANCODE_E)) {
        info.brush_size -= 10.0f*dt;
        info.brush_size = std::max(info.brush_size, 2.0f);
    }
    if (im->key_pressed(SDL_SCANCODE_V)) {
        graphics::set_vsync(!graphics::is_vsync());
    }

    // make canvas bigger
    // TODO: revisit this later. We will recreate texture many many times as long as we are dragging.
    //       maybe better option would be to drag and only resize onRelease. 
    //       We can draw outline for "new" bounds
    if (info.mouse_moved && 
        im->mouse_down(mouse_button::MID) &&
        im->key_down(SDL_SCANCODE_LSHIFT)) {

        const auto rel_motion {graphics::get_relative_motion()};
        const auto dx {rel_motion.x};
        const auto dy {rel_motion.y};
        std::println("dx {} dy {}", dx, dy);

        const auto new_width {canvas.width + static_cast<int>(dx)};
        const auto new_height {canvas.height + static_cast<int>(dy)};

        gl::texture2d texture = graphics::create_texture2d(canvas.width, canvas.height, GL_RGBA8);

        temp_canvas.texture = graphics::create_texture2d(new_width, new_height, GL_RGBA8);
        glNamedFramebufferTexture(temp_canvas.buffer.id, GL_COLOR_ATTACHMENT0, temp_canvas.texture.id, 0);
        const auto status {glCheckNamedFramebufferStatus(temp_canvas.buffer.id, GL_FRAMEBUFFER)};
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::println("FrameBuffer with id {} is incomplete\n {}", temp_canvas.buffer.id, status);
        }
        temp_canvas.width = new_width;
        temp_canvas.height = new_height;
        info.resized = true;

        return;
    }

    // panning around
    if (info.mouse_moved && im->mouse_down(mouse_button::MID)) {
        const auto rel_motion {graphics::get_relative_motion()};
        info.world_offset_x += rel_motion.x;
        info.world_offset_y += rel_motion.y;
        return;
    }

    // if we are not resizing or panning around 
    // and if we click inside canvas then we are drawing
    // TODO: experiment with different approach for updating canvas.
    //       currently we are adding brush strokes (i.e. circles) to list. And on each frame rendering
    //       that list to canvas buffer. 
    //       It would be better to clear this list from time to time, and do not update canvas if we are not drawing on it.
    //       Basically clear or not to clear on each frame.
    //
    //       Maybe try to update parts of canvas as well? Like how would we do bucket tool
    {
        const auto canvas_world_center_x {canvas.pos_x+info.world_offset_x};
        const auto canvas_world_center_y {canvas.pos_y+info.world_offset_y};
        const auto canvas_lower_left_x   {canvas_world_center_x-canvas.width*0.5f};
        const auto canvas_lower_left_y   {canvas_world_center_y-canvas.height*0.5f};
        const auto canvas_upper_right_x  {canvas_world_center_x+canvas.width*0.5f};
        const auto canvas_upper_right_y  {canvas_world_center_y+canvas.height*0.5f};

        bool inside_canvas {mx >= canvas_lower_left_x &&
                            mx <= canvas_upper_right_x &&
                            my >= canvas_lower_left_y &&
                            my <= canvas_upper_right_y};

        if (im->mouse_down(mouse_button::LEFT) && inside_canvas) {
            cpx.emplace_back(mx-canvas_lower_left_x);
            cpy.emplace_back(my-canvas_lower_left_y);
            cpr.emplace_back(info.brush_size*0.5f);
            info.should_draw = true;
            info.should_empty = false;
        }

        if (im->mouse_released(mouse_button::LEFT) && inside_canvas) {
            cpx.emplace_back(mx-canvas_lower_left_x);
            cpy.emplace_back(my-canvas_lower_left_y);
            cpr.emplace_back(info.brush_size*0.5f);
            info.should_draw = true;
            info.should_empty = true;
        }
    }

    if (im->key_down(SDL_SCANCODE_LSHIFT) && im->mouse_pressed(mouse_button::LEFT)) {
        test_data.lines.push_back({mx, my});
    }
    if (im->key_down(SDL_SCANCODE_LSHIFT) && im->mouse_released(mouse_button::LEFT)) {
        test_data.lines.back().x2 = mx;
        test_data.lines.back().y2 = my;
        test_data.should_do = true;
    }

}

void application::draw()
{
    // TODO: Test clear vs non-clear approach.
    // We want to clear canvas to some background color when we first initialize.
    if (info.init) {
        graphics::bind_frame_buffer(canvas.buffer);
        graphics::set_viewport(0, 0, canvas.width, canvas.height);
        graphics::clear_buffer_color(canvas.buffer.id, canvas.bg_color);
        info.init = false;
    }

    if (info.resized) {
        graphics::bind_frame_buffer(temp_canvas.buffer);
        graphics::set_viewport(0, 0, temp_canvas.width, temp_canvas.height);
        graphics::clear_buffer_color(temp_canvas.buffer.id, canvas.bg_color);

        const auto w {std::min(temp_canvas.width, canvas.width)};
        const auto h {std::min(temp_canvas.height, canvas.height)};
        glCopyImageSubData(canvas.texture.id, GL_TEXTURE_2D , 0, 0, 0, 0,
                           temp_canvas.texture.id, GL_TEXTURE_2D, 0, 0, 0, 0, 
                           w, h, 1);

        temp_canvas.buffer = std::exchange(canvas.buffer, std::move(temp_canvas.buffer));
        temp_canvas.texture = std::exchange(canvas.texture, std::move(temp_canvas.texture));
        temp_canvas.width = std::exchange(canvas.width, std::move(temp_canvas.width));
        temp_canvas.height = std::exchange(canvas.height, std::move(temp_canvas.height));
        canvas.projection = math::get_ortho_projection(0.0f, static_cast<float>(canvas.width), 0.0f, static_cast<float>(canvas.height));

        info.resized = false;
    }

    // Only draw if we are doing brush strokes.
    if (info.should_draw) {
        graphics::bind_frame_buffer(canvas.buffer);
        graphics::set_viewport(0, 0, canvas.width, canvas.height);

        graphics::bind_vertex_array(vao);
        circle_shader.use_shader();

        std::println("{}", cpx.size());
        for (std::size_t i{}; i<cpx.size(); ++i) {
            math::mat4f m {math::translate(cpx[i], cpy[i], 0.0f)*
                           math::scale(cpr[i]*2, cpr[i]*2, 1.0f)};
            circle_shader.set_mat4("u_mvp", canvas.projection*m);
            circle_shader.set_float("u_radius", cpr[i]);
            circle_shader.set_vec2("u_center_world", cpx[i], cpy[i]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        if (info.should_empty) {
            cpx.clear();
            cpy.clear();
            cpr.clear();
        }
    }

    // Last pass to render canvas on a quad and position it in the world based on world pos and world offset.
    {
        graphics::bind_frame_buffer_default();
        graphics::clear_buffer_all(0, graphics::GREY, 1.0f, 0);
        graphics::set_viewport(0, 0, app_settings_.window_width, app_settings_.window_height);

        const auto cw {static_cast<float>(canvas.width)};
        const auto ch {static_cast<float>(canvas.height)};

        math::mat4f model {math::translate(canvas.pos_x+info.world_offset_x, canvas.pos_y+info.world_offset_y, 0.0f)*
                           math::scale(cw, ch, 1.0f)};

        graphics::bind_vertex_array(canvas_vao);
        textured_quad_shader.use_shader();
        textured_quad_shader.set_mat4("u_mvp", window_projection*model);
        graphics::bind_texture_and_sampler(canvas.texture, canvas.sampler, 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }
}

// THIS is purely for experimenting and testing shaders, different shapes,
// and different approaches
void application::test_draw()
{
    graphics::bind_frame_buffer_default();
    graphics::clear_buffer_all(0, graphics::TEAL, 1.0f, 0);
    graphics::set_viewport(0, 0, app_settings_.window_width, app_settings_.window_height);

    const auto [mx, my] {input_manager::instance()->get_mouse_gl()};
    const auto r {5.0f};

    graphics::bind_vertex_array(vao);
    circle_shader.use_shader();

    math::mat4f model {math::translate(mx, my, 0.0f)*
                       math::scale(r*2, r*2, 1.0f)};
    circle_shader.set_mat4("u_mvp", window_projection*model);
    circle_shader.set_float("u_radius", r);
    circle_shader.set_vec2("u_center_world", mx, my);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    graphics::bind_vertex_array(line_vao);
    line_shader.use_shader();
    line_shader.set_vec4("u_line_color", graphics::color{0.54f, 0.52f, 0.2f, 1.0f});
    line_shader.set_mat4("u_mvp", window_projection);

    if (test_data.should_do) {
        std::vector<gl::vertex<gl::pos2>> ls; ls.reserve(test_data.lines.size());
        for (std::size_t i{}; i<test_data.lines.size(); ++i) {
            ls.push_back({{test_data.lines[i].x1, test_data.lines[i].y1}});
            ls.push_back({{test_data.lines[i].x2, test_data.lines[i].y2}});
        }
        line_vbo = gl::named_buffer{};
        graphics::buffer_upload_data(line_vbo, ls, GL_STATIC_DRAW);
        graphics::vao_configure<gl::pos2>(line_vao, line_vbo, 0);
        std::println("{}", ls.size());
        test_data.should_do = false;
    }

    glLineWidth(11.0f);
    glDrawArrays(GL_LINES, 0, static_cast<int>(test_data.lines.size()*2));
}



}
