#version 460 core

in vec2 frag_pos;
in vec2 p1;
in vec2 p2;
in vec3 color;
in float thickness;
in float aa;

out vec4 fragment_color;

void main()
{
    vec2 target = frag_pos-p1;
    vec2 target2 = frag_pos-p2;

    vec2 line = p2-p1;
    float line_len = length(line);
    vec2 line_dir = normalize(line);
    vec2 line_dir_90 = vec2(-line_dir.y, line_dir.x);

    vec2 line_side = thickness*line_dir_90;
    float line_side_len = length(line_side);

    vec2 projection_on_line = (dot(target, line)/(line_len*line_len))*line;
    vec2 projection_on_line_rev = (dot(target2, -line)/(line_len*line_len))*(-line);
    vec2 projection_on_side = (dot(target, line_side)/(line_side_len*line_side_len))*line_side;
    
    float s1 = smoothstep(line_len, line_len-aa, length(projection_on_line));
    float s2 = smoothstep(line_side_len, line_side_len-aa, length(projection_on_side));
    float s3 = smoothstep(line_len, line_len-aa, length(projection_on_line_rev));

    fragment_color = vec4(color*s1*s2*s3, s1*s2*s3);
}
