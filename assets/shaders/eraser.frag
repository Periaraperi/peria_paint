#version 460 core

out vec4 final_color;

uniform float u_radius;
uniform vec2 u_center;

void main()
{
    vec2 p = gl_FragCoord.xy;
    p -= u_center;
    float plen = length(p);
    float s = 1.0f - step(u_radius, plen);
    final_color = vec4(0.0f, 0.0f, 0.0f, s);
}

