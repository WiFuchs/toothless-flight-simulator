#version 330 core
flat in vec3 fragNor;
uniform vec3 lightDir;
out vec4 Outcolor;

void main() {
    vec3 normal = normalize(fragNor);
    vec3 light = normalize(lightDir);
    vec3 color = vec3(0.05, 0.5, 0.2);
    //diffuse
    vec3 col = color * max(0, dot(normal, light));
    //ambient
    col += color * 0.15;
    Outcolor = vec4(col, 1.0);
}


