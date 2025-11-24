#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outColor;


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

    // Normalize per-fragment
    vec3 normal = normalize(fragNormalWorld);

    // Light direction (world space)
    vec3 lightDir = normalize(lightPos - fragPosWorld);

    // View direction
    vec3 viewDir = normalize(camPos - fragPosWorld);

    // --- Blinn–Phong
    vec3 halfDir = normalize(lightDir + viewDir);
    float specular = specularStrength * pow(max(dot(normal, halfDir), 0.0), shiny);

    // Lighting components
    float ambient = ambientStrength;
    float diffuse = diffuseStrength * max(dot(normal, lightDir), 0.0);

    float lightIntensity = ambient + diffuse + specular;

    outColor = vec4(lightIntensity * fragColor, 1.0);
}