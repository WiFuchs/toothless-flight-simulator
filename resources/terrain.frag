#version 330 core
layout(location = 0) out vec4 color_out;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;
layout(location = 3) out vec4 mat_out;

flat in vec3 fragNor;
in vec3 WorldPos;

void main() {
    //vec3 normal = normalize(fragNor);
    //vec3 light = normalize(lightDir);
    //vec3 color = vec3(0.05, 0.5, 0.2);
    //diffuse
    //vec3 col = color * max(0, dot(normal, light));
    //ambient
    //col += color * 0.15;
    //Outcolor = vec4(col, 1.0);

    color_out = vec4(0.05, 0.5, 0.2, 1.0);
    pos_out = vec4(WorldPos, 1.0);
    norm_out = vec4(normalize(fragNor), 1.0);
    mat_out = vec4(1.0, 0, 0.15, 0);
}


