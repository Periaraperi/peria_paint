#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aCenter;
layout (location = 3) in vec2 aRadii;

uniform mat4 u_mvp;
out vec2 center;
out vec3 color;
out float radius;

void main()
{
    gl_Position = u_mvp*vec4(aPos.xy, 0.0f, 1.0f);
    center = aCenter;
    color = aColor;
    radius = aRadii.x;
}
