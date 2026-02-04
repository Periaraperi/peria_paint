#pragma once

#include "math/matrix.hpp"
#include "math/vec.hpp"

namespace peria::math {

struct camera2d {
    camera2d() = default;

    void update(const mat4f& projection);

    vec2f pos {0.0f, 0.0f};
    float zoom_scale {1.0f};
    mat4f view {1.0f};
private:
    [[nodiscard]]
    vec2f screen_to_world(const vec2f& p, const mat4f& projection);
};

}

