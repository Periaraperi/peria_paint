#version 460 core

in vec2 texture_coordinates;
out vec4 fragment_color;

uniform sampler2D u_canvas_texture;
uniform float u_temp_toggle;

vec3 linear_to_srgb(vec3 c)
{
    return pow(c, vec3(1.0f/2.2f));
}

void main()
{
    vec3 color = texture(u_canvas_texture, texture_coordinates).rgb*u_temp_toggle;
    fragment_color = vec4(linear_to_srgb(color), 1.0f);
}
