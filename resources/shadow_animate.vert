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
out vec4 shadow;

uniform mat4 model;
uniform mat4 lightProjection;
uniform mat4 lightView;

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
    gl_Position = lightProjection * lightView * mTrans * vec4(aPos, 1.0);
}
