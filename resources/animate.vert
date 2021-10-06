#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIds;
layout (location = 6) in vec4 aBoneWeights;

out vec2 TexCoords;
out mat3 TBN;
out vec3 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout (std140) uniform AnimationBlock {
    mat4 animationTransforms[110];
};


void main()
{
    // accumulate animation transforms from each bone affecting this vertex
    mat4 trans = animationTransforms[aBoneIds[0]] * aBoneWeights[0];
    trans += animationTransforms[aBoneIds[1]] * aBoneWeights[1];
    trans += animationTransforms[aBoneIds[2]] * aBoneWeights[2];
    trans += animationTransforms[aBoneIds[3]] * aBoneWeights[3];
    
    mat4 mTrans = model * trans;
    
    vec3 normal = normalize((mTrans * vec4(aNormal, 0.0)).xyz);
    vec3 tangent = normalize((mTrans * vec4(aTangent, 0.0)).xyz);
    vec3 bitangent = normalize((mTrans * vec4(aBitangent, 0.0)).xyz);
    TBN = mat3(tangent, bitangent, normal);
    
    TexCoords = aTexCoords;
    gl_Position = projection * view * mTrans * vec4(aPos, 1.0);
    WorldPos = (mTrans * vec4(aPos, 1.0)).xyz;
}
