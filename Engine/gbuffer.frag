#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outMaterial;


//set 0 - Global
//set 1 - Resources

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec4 camPos;
} globalUbo;

struct ObjectSSBO {
    mat4 model;
};

layout(set = 0, binding = 1) buffer ObjectBuffer {
    ObjectSSBO objectSSBOs[];
};

layout(set = 1, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform Push {
    uint uboIndex;
    uint textureIndex;
} push;



void main() {
    //outAlbedo = vec4(fragColor, 1.0);
    outAlbedo = texture(textures[push.textureIndex], fragUV);
    outNormal = vec4(normalize(fragNormalWorld) * 0.5 + 0.5, 1.0);
    outMaterial = vec4(0.5, 0.0, 0.0 ,1.0);
}