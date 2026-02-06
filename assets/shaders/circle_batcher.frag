#version 460 core

out vec4 final_color;

in vec2 center;
in vec3 color;
in float radius;

void main()
{
    vec2 p = gl_FragCoord.xy;
    p -= center;
    float plen = length(p);
    float s = smoothstep(radius, radius-1.0f, plen);
    final_color = vec4(color, s);
}
