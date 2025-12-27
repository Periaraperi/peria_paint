#version 460 core

out vec4 final_color;

uniform float u_radius;
uniform vec2 u_center_world;
uniform vec4 u_color;

void main()
{
    vec2 p = gl_FragCoord.xy;
    p -= u_center_world;
    float s = smoothstep(u_radius, u_radius-1.0f, length(p));
    //float s = 1.0f - step(u_radius, length(p));
    //final_color = vec4(u_color.r, u_color.g, u_color.b, s);
    final_color = vec4(u_color.rgb*s, s);
}

