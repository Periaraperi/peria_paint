#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <print>

#include "app.hpp"
#include "math/matrix.hpp"

int main([[maybe_unused]]int argc, [[maybe_unused]]char** argv)
{
    //peria::math::mat4f a {2.0f};

    //peria::math::mat4f b {1.0f};
    //auto c {b*a};
    //std::println("{}", c.to_string());

    //peria::math::matrix<float, 2, 3> x{};
    //x(0, 2) = 12;

    //std::println("{}", x.to_string());

    peria::application app {peria::application_settings{"peria_paint", 800, 600, true}};
    if (app.initialized()) {
        app.run();
    }
}
