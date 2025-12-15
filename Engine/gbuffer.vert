#version 450
#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

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


void main() {

    mat4 model = ubos[nonuniformEXT(push.uboIndex)].model;
    mat4 view = ubos[nonuniformEXT(push.uboIndex)].view;
    mat4 projection = ubos[nonuniformEXT(push.uboIndex)].projection;
    vec4 lightPos = ubos[nonuniformEXT(push.uboIndex)].lightPos;
    vec4 lightDir = ubos[nonuniformEXT(push.uboIndex)].lightDir;
    vec4 camPos = ubos[nonuniformEXT(push.uboIndex)].camPos;



    vec4 positionWorld = model * vec4(inPosition, 1.0);
    gl_Position = projection * view * positionWorld;
    
    fragNormalWorld = normalize(mat3(model) * inNormal);
    fragPosWorld = positionWorld.xyz;

    fragColor = inColor;
}