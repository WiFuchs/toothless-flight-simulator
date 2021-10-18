#version 330 core
layout(location = 0) out vec4 color_out;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;
layout(location = 3) out vec4 mat_out;

in vec2 TexCoords;
in vec3 FragNor;
in vec3 WorldPos;

uniform sampler2D texture_diffuse1;

void main()
{
    // Calculate normal from normal map
    vec3 fragNor = normalize(FragNor);
    
    color_out = vec4(1.0, 1.0, 1.0, 1.0);
    pos_out = vec4(WorldPos, 1.0);
    norm_out = vec4(fragNor, 1.0);
    mat_out = vec4(0.0, 100.0, 0, 0);
}