#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 viewDirection;
layout(location = 0) out vec4 outColor;

//set 0 - Global
//set 1 - Resources
//set 2 - Targets

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec4 camPos;
} globalUbo;

layout(set = 1, binding = 1) uniform samplerCube skyboxSamplers[];
layout(set = 2, binding = 5) uniform samplerCube irradianceMap;
layout(set = 2, binding = 6) uniform samplerCube prefilterMap;

layout(push_constant) uniform Push {
    uint uboIndex;
    uint skyboxIndex;
} push;


void main() {
    vec3 dir = normalize(viewDirection);
    outColor = texture(skyboxSamplers[push.skyboxIndex], dir);
    //outColor = texture(irradianceMap, dir);
    //outColor = textureLod(prefilterMap, dir, 3.0);
}