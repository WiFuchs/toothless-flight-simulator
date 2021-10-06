#version 330 core
layout(location = 0) out vec4 color_out;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;
layout(location = 3) out vec4 mat_out;

in vec2 TexCoords;
in mat3 TBN;
in vec3 WorldPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;

void main()
{
    // Calculate normal from normal map
    vec3 fragNor = texture(texture_normal1, TexCoords).xyz;
    fragNor = normalize(2.0 * fragNor - 1.0);
    fragNor = normalize(TBN * fragNor);
    
    color_out = vec4(texture(texture_diffuse1, TexCoords).rgb, 1.0);
    pos_out = vec4(WorldPos, 1.0);
    norm_out = vec4(fragNor, 1.0);
    mat_out = vec4(1.0, 1.0, 0, 0);
}
