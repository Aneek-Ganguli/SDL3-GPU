#version 460

layout(location=0) in vec4 color;
layout(location=1) in vec2 outTexcoord;

layout(location=0) out vec4 frag_color;

layout(set=2,binding=0) uniform sampler2D textureSampler ;

void main(){
	frag_color = texture(textureSampler, outTexcoord) * color;	
}
