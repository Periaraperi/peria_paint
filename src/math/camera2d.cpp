#include "camera2d.hpp"
#include "graphics/graphics.hpp"
#include "input_manager.hpp"

namespace peria::math {

vec2f camera2d::screen_to_world(const vec2f& p, const mat4f& projection)
{
    const auto [sw, sh] {graphics::get_screen_size()};
    const auto clip {vec2f{2.0f*p.x / sw - 1.0f,
                           2.0f*p.y / sh - 1.0f}};
    const auto world {inverse(projection*view)*vec4f{clip.x, clip.y, 0.0f, 1.0f}};
    //const auto world {glm::inverse(view)*view_space};
    return {world.x, world.y};
}

void camera2d::update(const mat4f& projection)
{
    auto im {input_manager::instance()};
    vec2f mouse_screen {};
    {
        auto m {im->get_mouse_gl()};
        mouse_screen = {m.x, m.y};
    }

    if (im->mouse_down(mouse_button::LEFT) && im->mouse_moving()) {
        const auto rel {graphics::get_relative_motion()};
        pos -= (vec2f{rel.x, rel.y}*(1.0f/zoom_scale));
    }
    const auto mouse_before_zoom_world {screen_to_world(mouse_screen, projection)};
    bool zoomed {};
    if (im->key_pressed(SDL_SCANCODE_1)) {
        zoom_scale /= 2;
        zoomed = true;
    }
    if (im->key_pressed(SDL_SCANCODE_2)) {
        zoom_scale *= 2;
        zoomed = true;
    }
    if (zoomed) {
        // intermediate update to get view with new zoom
        view = mat4f{scale(zoom_scale, zoom_scale, 1.0f)*
                     translate(-pos.x, -pos.y, 0.0f)};
        const auto mouse_after_zoom_world {screen_to_world(mouse_screen, projection)};
        pos += (mouse_before_zoom_world-mouse_after_zoom_world);
    }

    // update view because of position change.
    view = mat4f{scale(zoom_scale, zoom_scale, 1.0f)*
                 translate(-pos.x, -pos.y, 0.0f)};
}

}

