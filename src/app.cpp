#include "app.hpp"

#include <SDL3/SDL.h>
#include <chrono>
#include <cmath>
#include <glad/glad.h>

#include <print>
#include <utility>

#include "input_manager.hpp"
#include "graphics/graphics.hpp"

namespace {

constexpr bool TESTING {false};
constexpr int MAX_FPS {500};
float uk {0.0f};
float RADIUS {10.0f};
std::vector<peria::brush_point> ps;

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
        0.5f * (c1*points[p0].x + c2*points[p1].x + c3*points[p2].x + c4*points[p3].x),
        0.5f * (c1*points[p0].y + c2*points[p1].y + c3*points[p2].y + c4*points[p3].y),
        r
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

application::application(application_settings&& settings)
    :app_settings_{std::move(settings)},
     sdl_initializer_{app_settings_},
     circle_shader{"./assets/shaders/circle.vert", "./assets/shaders/circle.frag"},
     textured_quad_shader{"./assets/shaders/quad.vert", "./assets/shaders/quad.frag"},
     line_shader{"./assets/shaders/line_v2.vert", "./assets/shaders/line_v2.frag"},
     canvas{gl::texture2d{graphics::create_texture2d(static_cast<int>(settings.window_width*0.75f), static_cast<int>(settings.window_height*0.75f), GL_RGBA8)},
            gl::sampler{graphics::create_sampler(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER)}, {}, 
            static_cast<int>(settings.window_width*0.75f), static_cast<int>(settings.window_height*0.75f), {}, {}, {}, graphics::WHITE},
     temp_canvas{{gl::texture2d{graphics::create_texture2d(canvas.width, canvas.height, GL_RGBA8)}}, {}, canvas.width, canvas.height}
{
    if (!sdl_initializer_.initialized) return;
    std::println("application construction");

    input_manager::initialize();

    {
        peria::graphics::set_vsync(true);
        peria::graphics::set_relative_mouse(sdl_initializer_.window, false);
        peria::graphics::set_screen_size(app_settings_.window_width, app_settings_.window_height);
        window_projection = math::get_ortho_projection(0.0f, static_cast<float>(app_settings_.window_width), 0.0f, static_cast<float>(app_settings_.window_height));
    }

    // GL entities here
    {
        std::array<u32, 6> indices {0,1,2, 0,2,3};

        std::array<gl::vertex<gl::pos2, gl::pos2, gl::color4, gl::attr<float, 1>>, 4> circle_data {{
            {{-0.5f, -0.5f}, {0, 0}, {0.0f, 0.0f, 0.0f, 1.0f}, {0}},
            {{-0.5f,  0.5f}, {0, 0}, {0.0f, 0.0f, 0.0f, 1.0f}, {0}},
            {{ 0.5f,  0.5f}, {0, 0}, {0.0f, 0.0f, 0.0f, 1.0f}, {0}},
            {{ 0.5f, -0.5f}, {0, 0}, {0.0f, 0.0f, 0.0f, 1.0f}, {0}},
        }};
        graphics::buffer_upload_data(circle_vbo, circle_data, GL_STATIC_DRAW);
        graphics::buffer_upload_data(quad_ibo, indices, GL_STATIC_DRAW);
        graphics::vao_configure<gl::pos2, gl::pos2, gl::color4, gl::attr<float, 1>>(circle_vao, circle_vbo, 0);
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
            graphics::buffer_upload_data(line_ibo, line_indices, GL_STATIC_DRAW);
            graphics::vao_configure<gl::pos2, gl::color4>(line_vao, line_vbo, 0);
            graphics::vao_connect_ibo(canvas_vao, line_ibo);
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
        canvas.pos_x = 0.5f*(static_cast<float>(graphics::get_screen_size().w));
        canvas.pos_y = 0.5f*(static_cast<float>(graphics::get_screen_size().h));
    }

    brush_points.reserve(2048);
    // times 4 because each line is represented as a quad which is 2 triangles with total of 4 verts
    line_batcher.lines_data.reserve(MAX_PER_BATCH*4);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        
        if (TESTING) {
            test_draw();
        }
        else {
            draw();
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

    if (TESTING) {
        if (im->mouse_pressed(mouse_button::LEFT)) {
            ps.emplace_back(brush_point{mx, my, RADIUS});
        }
        if (im->key_down(SDL_SCANCODE_W)) {
            RADIUS += 100.0f*dt;
            RADIUS = std::min(RADIUS, 250.0f);
        }
        if (im->key_down(SDL_SCANCODE_E)) {
            RADIUS -= 100.0f*dt;
            RADIUS = std::max(RADIUS, 2.0f);
        }
        if (im->key_down(SDL_SCANCODE_LSHIFT) && im->key_down(SDL_SCANCODE_Q)) {
            uk -= 100.0f*dt;
            uk = std::max(0.0f, uk);
        }
        else if (im->key_down(SDL_SCANCODE_Q)) {
            uk += 100.0f*dt;
        }
        return;
    }

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
        return; // we don't want to draw while panning around
    }

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
            brush_points.emplace_back(brush_point{mx-canvas_lower_left_x, my-canvas_lower_left_y, info.brush_size*0.5f});
            info.should_draw = true;
            info.should_empty = false;
        }

        if (im->mouse_released(mouse_button::LEFT) && inside_canvas) {
            brush_points.emplace_back(brush_point{mx-canvas_lower_left_x, my-canvas_lower_left_y, info.brush_size*0.5f});
            info.should_draw = true;
            info.should_empty = true;
        }
    }
}

void application::draw()
{
    // Clear resized buffer and copy all contents from old to new.
    // Then swap struct info as well.
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

        graphics::bind_vertex_array(circle_vao);
        circle_shader.use_shader();

        circle_shader.set_vec4("u_color", graphics::color{0,0,0,1});
        //std::println("{}", brush_points.size());

        // Draw brush stroke points
        for (std::size_t i{}; i<brush_points.size(); ++i) {
            const auto& [x, y, r] {brush_points[i]};
            math::mat4f m {math::translate(x, y, 0.0f)*
                           math::scale(2*r, 2*r, 1.0f)};
            circle_shader.set_mat4("u_mvp", canvas.projection*m);
            circle_shader.set_float("u_radius", r);
            circle_shader.set_vec2("u_center_world", x, y);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        std::vector<brush_point> samples;
        for (float t{}; t<static_cast<float>(brush_points.size())-3.0f; t+=0.1f) {
            samples.emplace_back(get_point_on_path(brush_points, t));
        }

        // Draw interpolated sample points on stroke points
        for (std::size_t i{}; i<samples.size(); ++i) {
            math::mat4f m {math::translate(samples[i].x, samples[i].y, 0.0f)*
                           math::scale(samples[i].r*2, samples[i].r*2, 1.0f)};
            circle_shader.set_mat4("u_mvp", canvas.projection*m);
            circle_shader.set_float("u_radius", samples[i].r);
            circle_shader.set_vec2("u_center_world", samples[i].x, samples[i].y);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        // Draw lines between interpolated points

        //DRAWS LINES BETWEEN MOUSE POINTS

        auto add_line = [this](const brush_point& p1, const brush_point& p2) {
            const auto& x1 {p1.x};
            const auto& y1 {p1.y};
            const auto& t1 {p1.r};
            const auto& x2 {p2.x};
            const auto& y2 {p2.y};
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

            line_batcher.lines_data.push_back({{lower_left_x,  lower_left_y }, {0.0f, 0.0f, 0.0f, 1}});
            line_batcher.lines_data.push_back({{upper_left_x,  upper_left_y }, {0.0f, 0.0f, 0.0f, 1}});
            line_batcher.lines_data.push_back({{upper_right_x, upper_right_y}, {0.0f, 0.0f, 0.0f, 1}});
            line_batcher.lines_data.push_back({{lower_right_x, lower_right_y}, {0.0f, 0.0f, 0.0f, 1}});
        };

        for (std::size_t i{1}; i<samples.size(); ++i) {
            const auto& x1 {samples[i-1].x};
            const auto& y1 {samples[i-1].y};
            const auto& t1 {samples[i-1].r};
            const auto& x2 {samples[i].x};
            const auto& y2 {samples[i].y};
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

            line_batcher.lines_data.push_back({{lower_left_x,  lower_left_y }, {0.0f, 0.0f, 0.0f, 1}});
            line_batcher.lines_data.push_back({{upper_left_x,  upper_left_y }, {0.0f, 0.0f, 0.0f, 1}});
            line_batcher.lines_data.push_back({{upper_right_x, upper_right_y}, {0.0f, 0.0f, 0.0f, 1}});
            line_batcher.lines_data.push_back({{lower_right_x, lower_right_y}, {0.0f, 0.0f, 0.0f, 1}});
        }
        if (brush_points.size() >= 2 && samples.size() >= 2) {
            add_line({brush_points[0].x, brush_points[0].y, brush_points[0].r}, samples[0]);
            add_line(samples.back(), {brush_points.back().x, brush_points.back().y, brush_points.back().r});
        }

        graphics::bind_vertex_array(line_vao);
        line_shader.use_shader();
        line_shader.set_mat4("u_mvp", canvas.projection);

        // TODO: FIX ME 
        int line_count {static_cast<int>((line_batcher.lines_data.size()/4))};
        std::size_t offset {};
        std::println("{} {}", line_count, line_batcher.lines_data.size());
        while (line_count > 0) {
            int c {}; // count for each batch
            if (line_count >= MAX_PER_BATCH) {
                c = MAX_PER_BATCH;
            }
            else { // smaller remaining part
                c = line_count;
            }
            std::println("ccc {}", c);

            glNamedBufferSubData(line_vbo.id, 0, static_cast<u32>(4*c)*line_batcher.lines_data[0].stride, line_batcher.lines_data.data()+offset);
            glDrawElements(GL_TRIANGLES, c*6, GL_UNSIGNED_INT, nullptr);
            offset += static_cast<u32>(4*c);
            line_count -= c;
        }
        //int iterations {line_count / MAX_PER_BATCH};
        //int leftover   {line_count - (MAX_PER_BATCH*iterations)};

        //for (int i{}; i<iterations; ++i) {
        //}
        //if (leftover > 0) {
        //    glNamedBufferSubData(line_vbo.id, 0, static_cast<u32>(4*leftover)*lb[0].stride, lb.data()+iterations*MAX_PER_BATCH*4);
        //    glDrawElements(GL_TRIANGLES, leftover*6, GL_UNSIGNED_INT, nullptr);
        //}
        line_batcher.lines_data.clear();

        if (info.should_empty) {
            brush_points.clear();
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

    graphics::bind_vertex_array(circle_vao);
    circle_shader.use_shader();

    circle_shader.set_vec4("u_color", graphics::color{0,0,0,1.0f});
    for (std::size_t i{}; i<ps.size(); ++i) {
        math::mat4f m {math::translate(ps[i].x, ps[i].y, 0.0f)*
                       math::scale(ps[i].r*2, ps[i].r*2, 1.0f)};
        circle_shader.set_mat4("u_mvp", window_projection*m);
        circle_shader.set_float("u_radius", ps[i].r);
        circle_shader.set_float("u_k", uk);
        circle_shader.set_vec2("u_center_world", ps[i].x, ps[i].y);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    /*
    circle_shader.set_vec4("u_color", graphics::color{0.5f,0.2f,0,1});

    auto get_point = [](float t) {
        // indices of 2 control points and 2 points on a curve
        std::size_t p1 {static_cast<std::size_t>(t)+1};
        std::size_t p2 {p1+1};
        std::size_t p3 {p2+1};
        std::size_t p0 {p1-1};

        t = t - (float)static_cast<int>(t);

        float tt {t*t};
        float ttt {tt*t};

        // contributions based on some predefined cubic functions
        float c1 {-ttt + 2.0f*tt - t};
        float c2 {3.0f*ttt - 5.0f*tt + 2.0f};
        float c3 {-3.0f*ttt + 4.0f*tt + t};
        float c4 {ttt-tt};

        return point {
            0.5f * (c1*ps[p0].x + c2*ps[p1].x + c3*ps[p2].x + c4*ps[p3].x),
            0.5f * (c1*ps[p0].y + c2*ps[p1].y + c3*ps[p2].y + c4*ps[p3].y),
        };
    };

    std::vector<point> samples;
    for (float t{}; t<static_cast<float>(ps.size())-3.0f; t+=0.05f) {
        samples.emplace_back(get_point(t));
    }

    //std::vector<point> samples;
    //for (float t{}; t<=1.0f; t+=0.005f) {
    //    for (std::size_t i{}; i+2<ps.size(); i+=2) {
    //        auto a {point{lerp(ps[i].x, ps[i+1].x, t), lerp(ps[i].y, ps[i+1].y, t)}};
    //        auto b {point{lerp(ps[i+1].x, ps[i+2].x, t), lerp(ps[i+1].y, ps[i+2].y, t)}};
    //        samples.emplace_back(point{lerp(a.x, b.x, t), lerp(a.y, b.y, t)});
    //    }
    //}

    graphics::bind_vertex_array(vao);
    circle_shader.use_shader();

    circle_shader.set_vec4("u_color", graphics::color{0,0,0,1});
    for (std::size_t i{}; i<ps.size(); ++i) {
        math::mat4f m {math::translate(ps[i].x, ps[i].y, 0.0f)*
                       math::scale(r*2, r*2, 1.0f)};
        circle_shader.set_mat4("u_mvp", window_projection*m);
        circle_shader.set_float("u_radius", r);
        circle_shader.set_vec2("u_center_world", ps[i].x, ps[i].y);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    circle_shader.set_vec4("u_color", graphics::color{0.5f,0.2f,0,1});
    float r2 {5.0f};
    for (std::size_t i{}; i<samples.size(); ++i) {
        math::mat4f m {math::translate(samples[i].x, samples[i].y, 0.0f)*
                       math::scale(r2*2, r2*2, 1.0f)};
        circle_shader.set_mat4("u_mvp", window_projection*m);
        circle_shader.set_float("u_radius", r2);
        circle_shader.set_vec2("u_center_world", samples[i].x, samples[i].y);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    std::vector<gl::vertex<gl::pos2, gl::color4>> ls; 
    ls.reserve(ps.size());

    for (std::size_t i{1}; i<samples.size(); ++i) {
        const auto& x1 {samples[i-1].x};
        const auto& y1 {samples[i-1].y};
        const auto& t1 {r2};
        const auto& x2 {samples[i].x};
        const auto& y2 {samples[i].y};
        const auto& t2 {r2};

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

        ls.push_back({{lower_left_x,  lower_left_y }, {0.5f,0.2f,0,1}});
        ls.push_back({{upper_left_x,  upper_left_y }, {0.5f,0.2f,0,1}});
        ls.push_back({{upper_right_x, upper_right_y}, {0.5f,0.2f,0,1}});
        ls.push_back({{lower_right_x, lower_right_y}, {0.5f,0.2f,0,1}});
    }

    line_v2_vbo = gl::named_buffer{};
    line_ibo = gl::named_buffer{};
    graphics::buffer_upload_data(line_v2_vbo, ls, GL_STATIC_DRAW);
    graphics::vao_configure<gl::pos2, gl::color4>(line_v2_vao, line_v2_vbo, 0);
    std::vector<u32> libo;
    for (std::size_t i{}, k{}; i<ls.size()/4; ++i, k+=4) {
        libo.emplace_back(k);
        libo.emplace_back(k+1);
        libo.emplace_back(k+2);

        libo.emplace_back(k);
        libo.emplace_back(k+2);
        libo.emplace_back(k+3);
    }

    graphics::buffer_upload_data(line_ibo, libo, GL_STATIC_DRAW);
    graphics::vao_connect_ibo(line_v2_vao, line_ibo);

    graphics::bind_vertex_array(line_v2_vao);
    line_v2_shader.use_shader();
    line_v2_shader.set_mat4("u_mvp", window_projection);
    glDrawElements(GL_TRIANGLES, static_cast<int>(libo.size()), GL_UNSIGNED_INT, nullptr);
    */

}

}
