#include "app.hpp"

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <print>
#include <utility>

#include "input_manager.hpp"
#include "graphics/graphics.hpp"

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
            400, 300, {}, {}, {}, graphics::AQUA},
     temp_canvas{{gl::texture2d{graphics::create_texture2d(400, 300, GL_RGBA8)}}, {}, 400, 300}
{
    if (!sdl_initializer_.initialized) return;
    std::println("application construction");

    input_manager::initialize();

    {
        //TODO: come back to these settings later
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        //glEnable(GL_STENCIL_TEST);
        //glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        peria::graphics::set_vsync(true);
        peria::graphics::set_relative_mouse(sdl_initializer_.window, false);
        peria::graphics::set_screen_size(app_settings_.window_width, app_settings_.window_height);
        window_projection = math::get_ortho_projection(0.0f, static_cast<float>(app_settings_.window_width), 0.0f, static_cast<float>(app_settings_.window_height));
    }
    // GL entities here
    {
        std::array<u32, 6> indices {0,1,2, 0,2,3};

        std::array<gl::vertex<gl::pos2, gl::pos2, gl::color4, gl::attr<float, 1>>, 4> circle_data {{
            {{-0.5f, -0.5f}, {0, 0}, {1.0f, 0.4f, 0.2f, 1.0f}, {0}},
            {{-0.5f,  0.5f}, {0, 0}, {1.0f, 0.4f, 0.2f, 1.0f}, {0}},
            {{ 0.5f,  0.5f}, {0, 0}, {1.0f, 0.4f, 0.2f, 1.0f}, {0}},
            {{ 0.5f, -0.5f}, {0, 0}, {1.0f, 0.4f, 0.2f, 1.0f}, {0}},
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

    while (running) {
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

        update();

        draw();

        if (!SDL_GL_SwapWindow(sdl_initializer_.window)) {
            std::println("SDL_GL_SwapWindow failed");
            running = false;
            break;
        }

        input_manager_->update_prev_state();
        info.mouse_moved = false;

        SDL_Delay(1); // artificial delay of 1ms to not go bonkers
    }
}

static float brush_size {10.0f};
static bool should_draw {false};
static bool init {true};
static bool resized {true};

void application::update()
{
    const auto im {input_manager::instance()};
    const auto [mx, my] {im->get_mouse_gl()};
    should_draw = false;

    if (im->key_down(SDL_SCANCODE_W)) {
        brush_size += 1.0f;
        brush_size = std::min(brush_size, 150.0f);
    }
    if (im->key_down(SDL_SCANCODE_E)) {
        brush_size -= 1.0f;
        brush_size = std::max(brush_size, 2.0f);
    }

    // make canvas bigger
    // TODO: revisit this later. We will recreate texture many many times as long as we are dragging.
    //       maybe better option would be to drag and only resize onRelease. 
    //       We can draw outline for "new" bounds
    if (info.mouse_moved && 
        im->mouse_down(mouse_button::MID) &&
        im->key_down(SDL_SCANCODE_LSHIFT)) {

        const auto rel_motion {graphics::get_relative_motion()};
        const auto dx {rel_motion.x*info.pan_speed};
        const auto dy {rel_motion.y*info.pan_speed};
        std::println("dx {} dy {}", dx, dy);

        const auto new_width {canvas.width + static_cast<int>(dx)};
        const auto new_height {canvas.height + static_cast<int>(dy)};

        //canvas.texture = graphics::create_texture2d(canvas.width, canvas.height, GL_RGBA8);
        gl::texture2d texture = graphics::create_texture2d(canvas.width, canvas.height, GL_RGBA8);
        //std::vector<u32> pixels;
        //pixels.reserve(static_cast<std::size_t>(new_width*new_height));
        //for (int i{}; i<new_width*new_height; ++i) {
        //    pixels.emplace_back(11);
        //}
        //glTextureSubImage2D(texture.id, 0, 0, 0, new_width, new_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        temp_canvas.texture = graphics::create_texture2d(new_width, new_height, GL_RGBA8);
        glNamedFramebufferTexture(temp_canvas.buffer.id, GL_COLOR_ATTACHMENT0, temp_canvas.texture.id, 0);
        const auto status {glCheckNamedFramebufferStatus(temp_canvas.buffer.id, GL_FRAMEBUFFER)};
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::println("FrameBuffer with id {} is incomplete\n {}", temp_canvas.buffer.id, status);
        }
        temp_canvas.width = new_width;
        temp_canvas.height = new_height;
        resized = true;

        //glCopyImageSubData(canvas.texture.id, GL_TEXTURE_2D , 0, 0, 0, 0,
        //                   texture.id, GL_TEXTURE_2D, 0, 0, 0, 0, 
        //                   canvas.width, canvas.height, 1);
        //canvas.texture = std::move(texture);
        //canvas.width = new_width;
        //canvas.height = new_height;
        //glNamedFramebufferTexture(canvas.buffer.id, GL_COLOR_ATTACHMENT0, canvas.texture.id, 0);
        //const auto status {glCheckNamedFramebufferStatus(canvas.buffer.id, GL_FRAMEBUFFER)};
        //if (status != GL_FRAMEBUFFER_COMPLETE) {
        //    std::println("FrameBuffer with id {} is incomplete\n {}", canvas.buffer.id, status);
        //}
        //canvas.projection = math::get_ortho_projection(0.0f, static_cast<float>(canvas.width), 0.0f, static_cast<float>(canvas.height));
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
        const auto canvas_world_center_x {canvas.pos_x+info.world_offset_x*info.pan_speed};
        const auto canvas_world_center_y {canvas.pos_y+info.world_offset_y*info.pan_speed};
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
            cpr.emplace_back(brush_size*0.5f);
            should_draw = true;
        }
    }

}

void application::draw()
{
    const auto im {input_manager::instance()};
    const auto [mx, my] {im->get_mouse_gl()};

    // TODO: Test clear vs non-clear approach.
    if (init) {
        graphics::bind_frame_buffer(canvas.buffer);
        graphics::set_viewport(0, 0, canvas.width, canvas.height);
        graphics::clear_buffer_color(canvas.buffer.id, canvas.bg_color);
        init = false;
    }

    if (resized) {
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

        resized = false;
    }

    // render to canvas framebuffer
    if (should_draw) {
        const auto canvas_world_center_x {canvas.pos_x+info.world_offset_x*info.pan_speed};
        const auto canvas_world_center_y {canvas.pos_y+info.world_offset_y*info.pan_speed};
        const auto canvas_lower_left_x   {canvas_world_center_x-canvas.width*0.5f};
        const auto canvas_lower_left_y   {canvas_world_center_y-canvas.height*0.5f};

        graphics::bind_frame_buffer(canvas.buffer);
        graphics::set_viewport(0, 0, canvas.width, canvas.height);
        //graphics::clear_buffer_color(canvas.buffer.id, graphics::AQUA);

        graphics::bind_vertex_array(vao);
        circle_shader.use_shader();

        for (std::size_t i{}; i<cpx.size(); ++i) {
            math::mat4f m {math::translate(cpx[i], cpy[i], 0.0f)*
                           math::scale(cpr[i]*2, cpr[i]*2, 1.0f)};
            circle_shader.set_mat4("u_mvp", canvas.projection*m);
            circle_shader.set_float("u_radius", cpr[i]);
            circle_shader.set_vec2("u_center_world", cpx[i], cpy[i]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }
        cpx.clear();
        cpy.clear();
        cpr.clear();
        
        // brush cursor
        //math::mat4f model {math::translate(mx-canvas_lower_left_x, my-canvas_lower_left_y, 0.0f)*
        //                   math::scale(brush_size, brush_size, 1.0f)};
        //circle_shader.set_mat4("u_mvp", canvas.projection*model);
        //circle_shader.set_float("u_radius", brush_size*0.5f);
        //circle_shader.set_vec2("u_center_world", mx-canvas_lower_left_x, my-canvas_lower_left_y);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    {
        graphics::bind_frame_buffer_default();
        graphics::clear_buffer_all(0, graphics::INDIGO, 1.0f, 0);
        graphics::set_viewport(0, 0, app_settings_.window_width, app_settings_.window_height);


        const auto cw {static_cast<float>(canvas.width)};
        const auto ch {static_cast<float>(canvas.height)};

        math::mat4f model {math::translate(canvas.pos_x+info.world_offset_x*info.pan_speed, canvas.pos_y+info.world_offset_y*info.pan_speed, 0.0f)*
                           math::scale(cw, ch, 1.0f)};

        graphics::bind_vertex_array(canvas_vao);
        textured_quad_shader.use_shader();
        textured_quad_shader.set_mat4("u_mvp", window_projection*model);
        graphics::bind_texture_and_sampler(canvas.texture, canvas.sampler, 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }
}

}
