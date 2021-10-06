#version  330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

flat out vec3 fragNor;
out vec3 WorldPos;

void main() {
    WorldPos = (model * vec4(vertPos.xyz, 1.0)).xyz;
    gl_Position = projection * view * model * vec4(vertPos.xyz, 1.0);
    fragNor = (model * vec4(vertNor, 0.0)).xyz;
}
