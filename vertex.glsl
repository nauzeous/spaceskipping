#version 430

layout (location=0) in vec2 aPos;
out vec2 z



void main(){
	gl_Position = vec4(aPos,0.,1.);

}