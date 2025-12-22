#version 450
#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;



//set 0 - Target

layout(set = 0, binding = 3) uniform sampler2D lightingImage;

layout(push_constant) uniform Push {
    uint uboIndex;
    uint textureIndex;
} push;

void main() {
	outColor = texture(lightingImage, fragUV);
}