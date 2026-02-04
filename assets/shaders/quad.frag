#version 460 core

in vec2 texture_coordinates;
out vec4 fragment_color;

uniform sampler2D u_canvas_texture;
uniform float u_temp_toggle;
uniform bool u_toggle;

vec4 linear_to_srgb(vec4 c)
{
    return pow(c, vec4(1.0f/2.2f));
}

void main()
{
    if (u_toggle) {
        fragment_color = vec4(1.0f, 0.5f, 0.8f, 1.0f);
    }
    else {
        vec4 color = texture(u_canvas_texture, texture_coordinates).rgba*u_temp_toggle;
        fragment_color = vec4(linear_to_srgb(color));
    }
}
