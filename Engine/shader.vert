#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

layout(binding = 0) uniform UBOLighting {
    vec3 lightPos;
    vec3 lightDir;
    vec3 camPos;
} ubo;

layout(push_constant) uniform Push {
    mat4 model;
    mat4 view;
    mat4 projection;
} push;


void main() {
    vec4 positionWorld = push.model * vec4(inPosition, 1.0);
    gl_Position = push.projection * push.view * positionWorld;
    
    fragNormalWorld = normalize(mat3(push.model) * inNormal);
    fragPosWorld = positionWorld.xyz;

    fragColor = inColor;
}