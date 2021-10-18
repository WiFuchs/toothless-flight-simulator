#version 330 core
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D colTex;
uniform sampler2D posTex;
uniform sampler2D norTex;
uniform sampler2D matTex;
uniform sampler2D shadowMap;
uniform sampler2D depthTexture;
uniform vec3 skyColor;
uniform vec3 campos;
uniform vec3 lightDir;
uniform mat4 lightProjection;
uniform mat4 lightView;

float calcShadowFactor(vec4 lightSpacePosition) {
    vec3 shifted = (lightSpacePosition.xyz / lightSpacePosition.w + 1.0) * 0.5;

    float shadowFactor = 0;
    float bias = 0.051;
    float fragDepth = shifted.z - bias;

    //return (fragDepth - textureOffset(shadowMap, shifted.xy, ivec2(0, 0)).r) * 100000;

    if (fragDepth > 1.0) {
        return 0.0;
    }
    if(shifted.x>=1)
        return 0.0;
    if(shifted.y>=1)
        return 0.0;
    if(shifted.x<0)
        return 0.0;
    if(shifted.y<0)
        return 0.0;

    if (fragDepth > texture(shadowMap, shifted.xy).r) {
            shadowFactor += 1;
        }

    return shadowFactor;
}

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
    float spec = pow(dot(h,normal),20);
    spec = spec * material.g;
    spec = clamp(spec,0,1);

    // ambient multiplier
    float ambient = 0.15 + material.b;
    return lightCol * spec;
    //return lightCol *(light + spec + ambient);
}

void main()
{
    vec3 col = texture(colTex, TexCoords).rgb;
	vec3 worldPos = texture(posTex, TexCoords).rgb;
	vec3 normal = texture(norTex, TexCoords).rgb;
    vec3 material = texture(matTex, TexCoords).rgb;

    FragColor = vec4(col, 1.0);
    return;

    // shadows
    vec4 lightscreenpos = lightProjection * lightView * vec4(worldPos,1);
	//FragColor = vec4(vec3(calcShadowFactor(lightscreenpos)), 1);
    //return;
    float shadowFactor = 1.0 - calcShadowFactor(lightscreenpos);
    col *= shadowFactor;
   
    // Lighting
    vec3 lightCol = calcDirectionalLight(lightDir, vec3(1,1,1), normal, worldPos, material);
    col *= lightCol;

    // Fog
//    float n = 1.0;
//    float f = 100.0f;
//    float z = texture(depthTexture, TexCoords).r;
//    float grey = (2.0 * n) / (f + n - z*(f-n));
//    col += pow(grey, 10.0f) * skyColor;
//    col.x = min(col.x, skyColor.x);
//    col.y = min(col.y, skyColor.y);
//    col.z = min(col.z, skyColor.z);

    FragColor = vec4(col, 1.0);
    //FragColor = vec4(texture(shadowMap, TexCoords).rrr, 1.0);
    
} 
