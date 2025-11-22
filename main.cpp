#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <print>

#include "app.hpp"

int main([[maybe_unused]]int argc, [[maybe_unused]]char** argv)
{
    peria::application app {peria::application_settings{"peria_paint", 1600, 900, true}};
    if (app.initialized()) {
        app.run();
    }
}
