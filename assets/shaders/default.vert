#version 460

layout (location = 0) in vec2 aPos;

uniform mat4 u_mvp;

void main()
{
    gl_Position = vec4(aPos, 0.0f, 1.0f);
}

