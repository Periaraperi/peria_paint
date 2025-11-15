#include "input_manager.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <print>

#include "graphics/graphics.hpp"

namespace peria {

input_manager::input_manager()
    :keys_length_{0},
     keyboard_state_{nullptr},
     prev_keyboard_state_{nullptr}
{
    // no need to call SDL_GetKeyboardState again, we poll events in main loop which
    // pumps events, which updates keyboard_state array. We just maintain pointer to it
    keyboard_state_ = SDL_GetKeyboardState(&keys_length_);
    prev_keyboard_state_ = new bool[static_cast<unsigned long>(keys_length_)];
    std::copy(keyboard_state_, keyboard_state_+keys_length_, prev_keyboard_state_);
    mouse_state_ = SDL_GetMouseState(&mouse_x_, &mouse_y_);
    
    std::print("input manager ctor()\n");
}

input_manager::~input_manager()
{
    std::print("input manager dtor()\n");
    delete[] prev_keyboard_state_; 
    prev_keyboard_state_ = nullptr;
}

void input_manager::initialize() noexcept
{
    if (!instance_ptr_) instance_ptr_ = new input_manager{};
    else std::println("input manager instance already initialized");
}

void input_manager::shutdown() noexcept
{ delete instance_ptr_; instance_ptr_ = nullptr; }

input_manager* input_manager::instance() noexcept
{ return instance_ptr_; }

void input_manager::update_prev_state()
{
    std::copy(keyboard_state_, keyboard_state_+keys_length_, prev_keyboard_state_);
    prev_mouse_state_ = mouse_state_;
}

void input_manager::update_mouse() noexcept
{ mouse_state_ = SDL_GetMouseState(&mouse_x_, &mouse_y_); }

mouse input_manager::get_mouse() const noexcept
{ return {mouse_x_, mouse_y_}; }

mouse input_manager::get_mouse_gl() const noexcept
{
    const auto h {graphics::get_screen_size().h};
    return {mouse_x_, static_cast<float>(h)-mouse_y_};
}

u32 input_manager::get_mask(mouse_button btn) const noexcept
{
    u32 mask {};
    switch (btn) {
        case mouse_button::LEFT:
            mask = SDL_BUTTON_LMASK;
            break;
        case mouse_button::RIGHT:
            mask = SDL_BUTTON_RMASK;
            break;
        case mouse_button::MID:
            mask = SDL_BUTTON_MMASK;
            break;
    }
    return mask;
}

bool input_manager::key_pressed(SDL_Scancode key) const noexcept
{ return (keyboard_state_[key] && !prev_keyboard_state_[key]); }

bool input_manager::key_down(SDL_Scancode key) const noexcept
{ return (keyboard_state_[key] && prev_keyboard_state_[key]); }

bool input_manager::key_released(SDL_Scancode key) const noexcept
{ return (!keyboard_state_[key] && prev_keyboard_state_[key]); }

bool input_manager::mouse_pressed(mouse_button btn) const noexcept
{
    auto mask {get_mask(btn)};
    return ((mouse_state_&mask)!=0 && (prev_mouse_state_&mask)==0);
}

bool input_manager::mouse_down(mouse_button btn) const noexcept
{
    auto mask {get_mask(btn)};
    return ((mouse_state_&mask)!=0 && (prev_mouse_state_&mask)!=0);
}

bool input_manager::mouse_released(mouse_button btn) const noexcept
{
    auto mask {get_mask(btn)};
    return ((mouse_state_&mask)==0 && (prev_mouse_state_&mask)!=0);
}

}
