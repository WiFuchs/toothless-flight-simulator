#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform vec3 skyColor;

void main()
{
    float n = 1.0;
    float f = 100.0f;
    float z = texture(depthTexture, TexCoords).r;
    float grey = (2.0 * n) / (f + n - z*(f-n));
    vec3 col = texture(screenTexture, TexCoords).rgb;
    col += pow(grey, 10.0f) * skyColor;
    col.x = min(col.x, skyColor.x);
    col.y = min(col.y, skyColor.y);
    col.z = min(col.z, skyColor.z);
    FragColor = vec4(col, 1.0);
} 
