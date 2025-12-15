#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outMaterial;


layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos;
    vec4 lightDir;
    vec4 camPos;
} ubos[];

layout(set = 0, binding = 1) uniform sampler2D textureSampler[]; 

layout(push_constant) uniform Push {
    uint uboIndex;
    uint textureIndex;
} push;


float ambientStrength = 0.1;
float diffuseStrength = 0.8;
float specularStrength = 0.5;
float shiny = 32.0;



void main() {

    mat4 model = ubos[nonuniformEXT(push.uboIndex)].model;
    mat4 view = ubos[nonuniformEXT(push.uboIndex)].view;
    mat4 projection = ubos[nonuniformEXT(push.uboIndex)].projection;
    vec3 lightPos = ubos[nonuniformEXT(push.uboIndex)].lightPos.xyz;
    //vec3 lightDir = ubos[nonuniformEXT(push.uboIndex)].lightDir.xyz;
    vec3 camPos = ubos[nonuniformEXT(push.uboIndex)].camPos.xyz;

    outAlbedo = vec4(fragColor, 1.0);
    outNormal = vec4(normalize(fragNormalWorld) * 0.5 + 0.5, 1.0);
    outMaterial = vec4(0.5, 0.0, 0.0 ,1.0);
}