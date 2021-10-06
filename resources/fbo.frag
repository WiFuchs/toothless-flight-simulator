#version 330 core
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D colTex;
uniform sampler2D posTex;
uniform sampler2D norTex;
uniform sampler2D matTex;
uniform sampler2D depthTexture;
uniform vec3 skyColor;
uniform vec3 campos;

vec3 calcDirectionalLight(vec3 lightDir, vec3 lightCol, vec3 normal, vec3 WorldPos, vec3 material) 
{
    //diffuse light
    vec3 ld = normalize(lightDir);
    float light = max(0, dot(ld, normal));
    light = light * material.r;
    light = clamp(light,0,1);

    //specular light
    vec3 camvec = normalize(campos - WorldPos);
	vec3 h = normalize(camvec+ld);
    float spec = pow(dot(h,normal),5);
    spec = spec * material.g;
    spec = clamp(spec,0,1);

    // ambient multiplier
    float ambient = 0.15 + material.b;

    return lightCol *(light + spec + ambient);
}

void main()
{
    vec3 col = texture(colTex, TexCoords).rgb;
	vec3 worldPos = texture(posTex, TexCoords).rgb;
	vec3 normal = texture(norTex, TexCoords).rgb;
    vec3 material = texture(matTex, TexCoords).rgb;

   
    // Lighting
    vec3 lightCol = calcDirectionalLight(vec3(100,100,100), vec3(1,1,1), normal, worldPos, material);
    col = col * lightCol;

    // Fog
    float n = 1.0;
    float f = 100.0f;
    float z = texture(depthTexture, TexCoords).r;
    float grey = (2.0 * n) / (f + n - z*(f-n));
    col += pow(grey, 10.0f) * skyColor;
    col.x = min(col.x, skyColor.x);
    col.y = min(col.y, skyColor.y);
    col.z = min(col.z, skyColor.z);

    FragColor = vec4(col, 1.0);

    
} 
