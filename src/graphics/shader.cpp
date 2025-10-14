#include "shader.hpp"

#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <print>

namespace peria::gl {

[[nodiscard]]
std::string parse_file(std::string path)
{
    // check for geometry shader
    if (path == "") return "";

    std::ifstream ifs{path};
    if (ifs.is_open()) {
        std::stringstream ss{};
        ss << ifs.rdbuf();
        return ss.str();
    }
    else {
        std::print("Error while opening file at path: {}\n", path);
    }
    return "";
}

[[nodiscard]]
u32 compile_shader(const char* src, u32 type) noexcept
{
	u32 shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	// check if successfully compiled
	int success;
	char log[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(shader, 512, nullptr, log);
        std::print("Couldn't compile shader\n {}\n", log);
	}

	return shader;
}

void create_shader_program(u32& id, u32 vertex_shader, u32 fragment_shader, u32 geometry_shader) noexcept
{
    id = glCreateProgram();
    glAttachShader(id, vertex_shader);
    glAttachShader(id, fragment_shader);
    if (geometry_shader != 0) {
        glAttachShader(id, geometry_shader);
    }
    glLinkProgram(id);

    // check if successfully linked
	int success;
	char log[512];
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(id, 512, nullptr, log);
        std::print("Couldn't link shader program\n {}\n", log);
	}
}

shader::shader(std::string vertex_path, std::string fragment_path, std::string geometry_path)
    :vertex_source{parse_file(std::move(vertex_path))},
     fragment_source{parse_file(std::move(fragment_path))},
     geometry_source{parse_file(std::move(geometry_path))}
{
    std::print("Creating Shader program\n");
    auto vertex_shader   {compile_shader(vertex_source.c_str(),   GL_VERTEX_SHADER)};
    auto fragment_shader {compile_shader(fragment_source.c_str(), GL_FRAGMENT_SHADER)};

    // opengl won't generate 0 for shader object handler,
    // so it can be used to determine if geometry shader is used or not.
    u32 geometry_shader {};
    if (!geometry_source.empty()) {
        geometry_shader = compile_shader(geometry_source.c_str(), GL_GEOMETRY_SHADER);
    }
    
    create_shader_program(id, vertex_shader, fragment_shader, geometry_shader);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    if (geometry_shader != 0) {
        glDeleteShader(geometry_shader);
    }
}

shader::~shader()
{
    std::print("Deleting Shader program\n");
    glDeleteProgram(id);
}

void shader::use_shader() const noexcept
{ glUseProgram(id); }

void shader::set_int(const char* u_name, int val) const noexcept
{ glProgramUniform1i(id, glGetUniformLocation(id, u_name), val); }

void shader::set_uint(const char* u_name, u32 val) const noexcept
{ glProgramUniform1ui(id, glGetUniformLocation(id, u_name), val); }

void shader::set_float(const char* u_name, float val) const noexcept
{ glProgramUniform1f(id, glGetUniformLocation(id, u_name), val); }

void shader::set_vec2(const char* u_name, float x, float y) const noexcept
{ glProgramUniform2f(id, glGetUniformLocation(id, u_name), x, y); }

void shader::set_vec4(const char* u_name, const graphics::color& v) const noexcept
{ glProgramUniform4f(id, glGetUniformLocation(id, u_name), v.r, v.g, v.b, v.a); }

void shader::set_mat4(const char* u_name, const math::mat4f& m, bool let_gl_transpose) const noexcept
{ glProgramUniformMatrix4fv(id, glGetUniformLocation(id, u_name), 1, let_gl_transpose, m.data()); }

void shader::set_array(const char* u_name, int count, int* arr) const noexcept 
{ glProgramUniform1iv(id, glGetUniformLocation(id, u_name), count, arr); }

}

