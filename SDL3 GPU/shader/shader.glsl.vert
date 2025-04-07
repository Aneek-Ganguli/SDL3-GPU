#version 460

layout(set=1,binding=0)uniform UBO{
	mat4 mvp;
};

layout(location=0) in vec3 position;

void main(){
	gl_Position = mvp * vec4(position,1);
}