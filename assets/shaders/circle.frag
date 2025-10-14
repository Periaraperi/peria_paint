#version 460

in vec4 color;
in vec2 center_world;
in float radius;
out vec4 final_color;

uniform float u_radius;
uniform vec2 u_center_world;

void main()
{
    vec2 p = gl_FragCoord.xy;
    p -= u_center_world; // translate to origin
    if (p.x*p.x+p.y*p.y <= u_radius*u_radius) {
        final_color = color;
    }
    //p -= center_world; // translate to origin
    //if (p.x*p.x+p.y*p.y <= radius*radius) {
    //    final_color = color;
    //}
    else {
        discard;
    }
}

