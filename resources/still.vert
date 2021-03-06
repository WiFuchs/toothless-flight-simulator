#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIds;
layout (location = 6) in vec4 aBoneWeights;

out vec2 TexCoords;
out vec3 FragNor;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

void main() {
    gl_Position = projection * view * model * vec4(aPos.xyz, 1.0);
    FragNor = (model * vec4(aPos, 0.0)).xyz;
    TexCoords = aTexCoords;
}
