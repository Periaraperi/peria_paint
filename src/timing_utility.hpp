#pragma once

#include <chrono>
namespace peria {

class game_loop_timer {
public:
    game_loop_timer(const game_loop_timer&) = delete;
    game_loop_timer& operator=(const game_loop_timer&) = delete;

    game_loop_timer(game_loop_timer&& rhs) = delete;
    game_loop_timer& operator=(game_loop_timer&& rhs) = delete;
    
    static void initialize() noexcept;
    static void shutdown() noexcept;

    [[nodiscard]]
    static game_loop_timer* instance() noexcept;

    void update() noexcept;

    [[nodiscard]]
    float dt() noexcept;

    [[nodiscard]]
    bool hit_target_frame_rate(float target) noexcept;
private:
    game_loop_timer();
    ~game_loop_timer();
    static inline game_loop_timer* game_loop_timer_instance {nullptr};

    float delta    {0.0f};         // in seconds
    float accum    {0.0f};         // in seconds

    std::chrono::steady_clock clock {};
    std::chrono::time_point<std::chrono::steady_clock> prev_time {clock.now()};
    std::chrono::time_point<std::chrono::steady_clock> curr_time {clock.now()};
};


}

