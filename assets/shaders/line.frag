#version 460

uniform vec4 u_line_color;

out vec4 fragment_color;

void main()
{
    fragment_color = u_line_color;
}


