#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outColor;


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



float ambientStrength = 0.1;
float diffuseStrength = 0.8;
float specularStrength = 0.5;
float shiny = 32.0;



void main() {

    // Normalize per-fragment
    vec3 normal = normalize(fragNormalWorld);

    // Light direction (world space)
    vec3 lightDir = normalize(ubo.lightPos - fragPosWorld);

    // View direction
    vec3 viewDir = normalize(ubo.camPos - fragPosWorld);

    // --- Blinn–Phong
    vec3 halfDir = normalize(lightDir + viewDir);
    float specular = specularStrength * pow(max(dot(normal, halfDir), 0.0), shiny);

    // Lighting components
    float ambient = ambientStrength;
    float diffuse = diffuseStrength * max(dot(normal, lightDir), 0.0);

    float lightIntensity = ambient + diffuse + specular;

    outColor = vec4(lightIntensity * fragColor, 1.0);
}