#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 ViewDir;
in mat3 TBN;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform vec3 lightColor;
uniform vec3 lightDir;

void main()
{
    // Calculate normal from normal map
    vec3 fragNor = texture(texture_normal1, TexCoords).xyz;
    fragNor = normalize(2.0 * fragNor - 1.0);
    fragNor = normalize(TBN * fragNor);
    
    vec3 light = normalize(lightDir);
    vec3 h = normalize(light + normalize(ViewDir));
    vec3 texColor = texture(texture_diffuse1, TexCoords).rgb;
    vec3 col = lightColor * texColor * max(0, dot(fragNor, light));
    col += lightColor * texColor * 0.15;
    col += lightColor * texColor * pow(dot(fragNor, h), 20.0f);
    
    FragColor = vec4(col, 1.0);
}
