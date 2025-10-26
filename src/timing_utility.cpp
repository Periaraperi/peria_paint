#include "timing_utility.hpp"

#include <print>

namespace peria {

game_loop_timer::game_loop_timer()
{ std::println("game_loop_timer ctor()"); }

game_loop_timer::~game_loop_timer()
{ std::println("game_loop_timer dtor()"); }


void game_loop_timer::initialize() noexcept
{ 
    if (game_loop_timer_instance == nullptr) {
        game_loop_timer_instance = new game_loop_timer{};
    }
}

void game_loop_timer::shutdown() noexcept
{ delete game_loop_timer_instance; game_loop_timer_instance = nullptr; }

game_loop_timer* game_loop_timer::instance() noexcept
{ return game_loop_timer_instance; }

void game_loop_timer::update() noexcept
{
    curr_time = clock.now();
    const auto elapsed_time {std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - prev_time).count()};
    delta = static_cast<float>(elapsed_time) * 0.001f;
    accum += delta;
    prev_time = curr_time;
}

float game_loop_timer::dt() noexcept
{ return delta; }

bool game_loop_timer::hit_target_frame_rate(float target) noexcept
{
    if (accum >= target) {
        accum = 0.0f;
        return true;
    }
    return false;
}

}

