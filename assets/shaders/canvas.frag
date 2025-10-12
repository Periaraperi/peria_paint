#version 460 core

in vec2 texture_coordinates;
out vec4 fragment_color;

uniform sampler2D u_canvas_texture;

void main()
{
    vec3 color = texture(u_canvas_texture, texture_coordinates).rgb;
    fragment_color = vec4(color, 1.0f);
}
