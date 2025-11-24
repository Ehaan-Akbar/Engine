#version 450
#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;



layout(set = 0, binding = 1) uniform sampler2D textureSampler[]; 

layout(push_constant) uniform Push {
    uint uboIndex;
    uint textureIndex;
} push;


void main() {
	outColor = texture(textureSampler[nonuniformEXT(push.textureIndex)], fragUV);
}