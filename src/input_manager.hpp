#pragma once

#include <SDL3/SDL_scancode.h>
#include <cstdint>

namespace peria {

using u32 = std::uint32_t;
using u8 = std::uint8_t;

struct mouse {
    float x {};
    float y {};
};

enum class mouse_button {
    LEFT = 0,
    RIGHT,
    MID
};

class input_manager {
public:
    input_manager(const input_manager&) = delete;
    input_manager operator=(const input_manager&) = delete;

    input_manager(input_manager&&) = delete;
    input_manager operator=(input_manager&&) = delete;

    static void initialize() noexcept;
    static void shutdown() noexcept;

    [[nodiscard]]
    static input_manager* instance() noexcept;

    [[nodiscard]]
    bool key_pressed(SDL_Scancode key) const noexcept;

    [[nodiscard]]
    bool key_down(SDL_Scancode key) const noexcept;

    [[nodiscard]]
    bool key_released(SDL_Scancode key) const noexcept;
 
    [[nodiscard]]
    bool mouse_pressed(mouse_button btn) const noexcept;

    [[nodiscard]]
    bool mouse_down(mouse_button btn) const noexcept;

    [[nodiscard]]
    bool mouse_released(mouse_button btn) const noexcept;

    [[nodiscard]]
    mouse get_mouse() const noexcept;

    [[nodiscard]]
    mouse get_mouse_gl() const noexcept;

    void update_prev_state();
    void update_mouse() noexcept;
private:
    int keys_length_;
    const bool* keyboard_state_;
    bool* prev_keyboard_state_;

    u32 mouse_state_;
    u32 prev_mouse_state_;

    float mouse_x_;
    float mouse_y_;

    static inline input_manager* instance_ptr_ {nullptr};

    input_manager();
    ~input_manager();

    [[nodiscard]]
    u32 get_mask(mouse_button btn) const noexcept;
};


}
