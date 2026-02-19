#version 460 core
layout (location = 0) in vec2 aPos;

uniform mat4 u_vp;
uniform mat4 u_model;
out vec2 frag_pos;

void main()
{
    vec4 world_pos = u_model*vec4(aPos.xy, 0.0f, 1.0f);
    gl_Position = u_vp*vec4(world_pos);
    frag_pos = world_pos.xy;
}

