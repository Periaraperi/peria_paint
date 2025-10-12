#version 460
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aCenterWorld;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aRadius;

uniform mat4 u_mvp;

out float radius;
out vec2 center_world;
out vec4 color;

void main()
{
    gl_Position = u_mvp*vec4(aPos.xy, 0.0f, 1.0f);
	color = aColor;
	center_world = aCenterWorld;
	radius = aRadius;
}

