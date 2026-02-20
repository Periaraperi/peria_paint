#version 460 core

in vec2 frag_pos;
out vec4 final_color;

uniform float u_radius2;
uniform float u_radius;

uniform vec2 u_center;
uniform vec3 u_color;

uniform bool u_is_ring;
uniform float u_aa;

void main()
{
    vec2 p = frag_pos;
    p -= u_center;
    float plen = length(p);
    if (u_is_ring) {
        float s1 = 1.0f - step(u_radius, plen);
        float s2 = step(u_radius2, plen);
        final_color = vec4(u_color.rgb*s1*s2, s1*s2);
    }
    else {
        float s = smoothstep(u_radius, u_radius-u_aa, plen);
        final_color = vec4(u_color.rgb*s, s);
    }
}

