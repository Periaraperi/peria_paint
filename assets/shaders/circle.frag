#version 460 core

in vec4 color;
in vec2 center_world;
in float radius;
out vec4 final_color;

uniform float u_radius;
uniform vec2 u_center_world;
uniform vec4 u_color;
uniform float u_k;

void main()
{
    vec2 p = gl_FragCoord.xy;
    p -= u_center_world;
    float s = 1.0f - smoothstep(u_radius-u_k, u_radius, length(p));
    final_color = u_color*vec4(s);
}

