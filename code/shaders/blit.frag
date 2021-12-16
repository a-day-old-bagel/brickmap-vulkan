#version 450

layout (set = 0,binding = 0) uniform sampler2D in_texture;
layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 out_color;

void main() 
{
	out_color = texture( in_texture, in_uv );
}

