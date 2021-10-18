#version 330 core
layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec3 vertNor;

uniform mat4 model;
uniform mat4 lightView;
uniform mat4 lightProjection;



void main()
{
    gl_Position = lightProjection * lightView * model * vec4(vertPos, 1.0);
    vertNor;
}
