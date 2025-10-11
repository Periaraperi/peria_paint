#include "gl_entities.hpp"

#include <glad/glad.h>

#include <print>
#include <utility>
#include <cstdint>

using u32 = std::uint32_t;

namespace peria::gl {

vertex_array::vertex_array() noexcept
{ std::println("vertex_array ctor()"); glCreateVertexArrays(1, &id); }

vertex_array::~vertex_array()
{ std::println("vertex_array ctor()"); glDeleteVertexArrays(1, &id); }

vertex_array::vertex_array(vertex_array&& rhs) noexcept
    :id{std::exchange(rhs.id, 0)}
{}

vertex_array& vertex_array::operator=(vertex_array&& rhs) noexcept
{
    if (&rhs == this) return *this;

    this->id = std::exchange(rhs.id, 0);
    return *this;
}

named_buffer::named_buffer() noexcept
{ std::println("named_buffer ctor()"); glCreateBuffers(1, &id); }

named_buffer::~named_buffer()
{ std::println("named_buffer dtor()"); glDeleteBuffers(1, &id); }

named_buffer::named_buffer(named_buffer&& rhs) noexcept
    :id{std::exchange(rhs.id, 0)}
{}

named_buffer& named_buffer::operator=(named_buffer&& rhs) noexcept
{
    if (&rhs == this) return *this;

    this->id = std::exchange(rhs.id, 0);
    return *this;
}

texture2d::texture2d() noexcept
{
    std::println("texture2d ctor()");
    glCreateTextures(GL_TEXTURE_2D, 1, &id);
}

texture2d::~texture2d()
{ std::println("Texture dtor()"); glDeleteTextures(1, &id); }

texture2d::texture2d(texture2d&& rhs) noexcept
    :id{std::exchange(rhs.id, 0)}
{}

texture2d& texture2d::operator=(texture2d&& rhs) noexcept
{
    if (&rhs == this) return *this;

    this->id = std::exchange(rhs.id, 0);
    return *this;
}

sampler::sampler() noexcept
{ std::println("sampler ctor()"); glCreateSamplers(1, &id); }

sampler::~sampler()
{ std::println("sampler dtor()"); glDeleteSamplers(1, &id); }

sampler::sampler(sampler&& rhs) noexcept
    :id{std::exchange(rhs.id, 0)}
{}

sampler& sampler::operator=(sampler&& rhs) noexcept
{
    if (&rhs == this) return *this;
    this->id = std::exchange(rhs.id, 0);
    return *this;
}

frame_buffer::frame_buffer() noexcept
{ std::println("frame_buffer ctor()"); glCreateFramebuffers(1, &id); }

frame_buffer::~frame_buffer()
{ std::println("frame_buffer dtor()"); glDeleteFramebuffers(1, &id); }

frame_buffer::frame_buffer(frame_buffer&& rhs) noexcept
    :id{std::exchange(rhs.id, 0)}
{}

frame_buffer& frame_buffer::operator=(frame_buffer&& rhs) noexcept
{
    if (&rhs == this) return *this;

    this->id = std::exchange(rhs.id, 0);
    return *this;
}

}
