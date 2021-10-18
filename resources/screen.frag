#version 450 core 

layout(location = 0) out vec4 color;

in vec2 fragTex;

layout(location = 0) uniform sampler2D colTex;
layout(location = 1) uniform sampler2D bloomTex;


void main()
{
	vec3 texturecolor = texture(colTex, fragTex).rgb;
	vec4 bloomcolor = texture(bloomTex, fragTex);

	color.rgb = texturecolor + bloomcolor.rgb * bloomcolor.a;
	color.a=1;
	
}
