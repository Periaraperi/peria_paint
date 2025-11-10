#version 460 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 u_mvp;

out vec4 color;

void main()
{
    gl_Position = u_mvp*vec4(aPos, 0.0f, 1.0f);
    color = aColor;
}
