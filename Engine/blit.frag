#version 450
#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;



//set 0 - Target

layout(set = 0, binding = 0) uniform sampler2D albedoImage;
layout(set = 0, binding = 1) uniform sampler2D normalImage;
layout(set = 0, binding = 2) uniform sampler2D materialImage;
layout(set = 0, binding = 3) uniform sampler2D lightingImage;
layout(set = 0, binding = 4) uniform sampler2D depthImage;
layout(set = 0, binding = 5) uniform samplerCube irradianceMap;
layout(set = 0, binding = 6) uniform samplerCube prefilterMap;
layout(set = 0, binding = 7) uniform sampler2D lutMap;


layout(push_constant) uniform Push {
    uint uboIndex;
    uint textureIndex;
} push;

void main() {
	outColor = texture(lightingImage, fragUV);
	//outColor = texture(materialImage, fragUV);
	//outColor = vec4(texture(lutMap, fragUV).xy, 0.0, 0.0);
}