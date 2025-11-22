#version 460 core

in vec4 color;
in vec2 center_world;
in float radius;
out vec4 final_color;

uniform float u_radius;
uniform vec2 u_center_world;
uniform vec4 u_color;

void main()
{
    vec2 p = gl_FragCoord.xy;
    p -= u_center_world;
    //float s = 1.0f - step(u_radius, length(p));
    float s = 1.0f - smoothstep(u_radius-6.5f, u_radius, length(p));
    float plen = length(p) / u_radius;
    //float s = max(0, 1.0f - step(1.0f, plen));
    final_color = u_color*vec4(s);
    //p -= u_center_world; // translate to origin
   
    /*
    if (p.x*p.x+p.y*p.y <= u_radius*u_radius) {
        final_color = color;
    }
    else {
        discard;
    }
    */
    
    //p -= center_world; // translate to origin
    //if (p.x*p.x+p.y*p.y <= radius*radius) {
    //    final_color = color;
    //}
    //else {
    //    discard;
    //}
}

