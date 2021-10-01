
#version 330 core
in vec3 FragNor;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec3 lightDir;

out vec4 Outcolor;

void main() {
    vec3 normal = normalize(FragNor);
    vec3 light = normalize(lightDir);
    vec3 color = texture(texture_diffuse1, TexCoords).xyz;
    //diffuse
    vec3 col = color * max(0, dot(normal, light));
    //ambient
    col += color * 0.15;
    
    Outcolor = vec4(col, 1.0);
}
