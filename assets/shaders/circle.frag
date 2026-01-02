#version 460 core

out vec4 final_color;

uniform float u_radius2;
uniform float u_radius;

uniform vec2 u_center_world;
uniform vec4 u_color;

uniform int u_is_ring;

void main()
{
    vec2 p = gl_FragCoord.xy;
    p -= u_center_world;
    float plen = length(p);
    if (u_is_ring == 1) {
        float s1 = 1.0f - step(u_radius, plen);
        float s2 = step(u_radius2, plen);
        final_color = vec4(u_color.rgb*s1*s2, s1*s2);
    }
    else {
        float s = smoothstep(u_radius, u_radius-1.0f, plen);
        final_color = vec4(u_color.rgb*s, s);
    }
}

