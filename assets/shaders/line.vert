#version 460 core

layout (location = 0) in vec2 aPos; // quad bounds already in world space
layout (location = 1) in vec4 aLineEndpoints; // (x1, y1, x2, y2)
layout (location = 2) in vec2 aThickAa; // (thickness, aa)
layout (location = 3) in vec3 aColor;

out vec2 frag_pos;
out vec2 p1;
out vec2 p2;
out vec3 color;
out float thickness;
out float aa;

uniform mat4 u_mvp;

void main()
{
    gl_Position = u_mvp*vec4(aPos, 0.0f, 1.0f);
    frag_pos = aPos;
    p1 = aLineEndpoints.xy;
    p2 = aLineEndpoints.zw;
    thickness = aThickAa.x;
    aa = aThickAa.y;
    color = aColor;
}
