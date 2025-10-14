#pragma once

#include <cstdint>
#include <string>

#include "graphics/color.hpp"
#include "math/matrix.hpp"

using u32 = std::uint32_t;

namespace peria::gl {

class shader {
public:
    shader() = default;
    shader(std::string vertex_path, std::string fragment_path, std::string geometry_path = "");

    shader(const shader&) = delete;
    shader& operator=(const shader&) = delete;

    shader(shader&&) noexcept = default;
    shader& operator=(shader&&) noexcept = default;

    void use_shader() const noexcept;

    ~shader();

    void set_int(const char* u_name, int val) const noexcept;
    void set_uint(const char* u_name, u32 val) const noexcept;
    void set_float(const char* u_name, float val) const noexcept;
    //void set_vec2(const char* u_name, const glm::vec2& v) const noexcept;
    //void set_vec3(const char* u_name, const glm::vec3& v) const noexcept;
    //void set_vec4(const char* u_name, const glm::vec4& v) const noexcept;
    void set_vec2(const char* u_name, float x, float y) const noexcept;
    void set_vec4(const char* u_name, const graphics::color& v) const noexcept;
    void set_mat4(const char* u_name, const math::mat4f& m, bool let_gl_transpose=true) const noexcept;
    void set_array(const char* u_name, int count, int* arr) const noexcept;

private: 
    u32 id;

    std::string vertex_source;
    std::string fragment_source;
    std::string geometry_source;
};

}
