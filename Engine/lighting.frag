#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

// Bindless texture array (G-buffer)
layout(set = 0, binding = 1) uniform sampler2D textures[];

// Frame UBO (only ONE used in lighting)
layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos;   // xyz = light position (world)
    vec4 lightDir;   // xyz = light direction (world)
    vec4 camPos;     // xyz = camera position (world)
} ubos[];

// Push constants (kept for future flexibility)
layout(push_constant) uniform Push {
    uint uboIndex;
} push;

// G-buffer indices
const uint GBUFFER_ALBEDO = 0;
const uint GBUFFER_NORMAL = 1;

void main()
{
    // Read G-buffer
    vec3 albedo = texture(
        textures[nonuniformEXT(GBUFFER_ALBEDO)],
        fragUV
    ).rgb;

    vec3 normal = texture(
        textures[nonuniformEXT(GBUFFER_NORMAL)],
        fragUV
    ).xyz * 2.0 - 1.0; // decode if stored in [0,1]

    vec4 material = texture(
        textures[nonuniformEXT(GBUFFER_NORMAL)],
        fragUV
    );

    normal = normalize(normal);

    vec3 lightPos = ubos[push.uboIndex].lightPos.xyz;

    // Fake fragment position (temporary hack)
    // For pipeline testing only
    vec3 fragPos = vec3(0.0);

    vec3 lightDir = normalize(lightPos - fragPos);

    // Diffuse lighting
    float diffuse = max(dot(normal, lightDir), 0.0);

    outColor = vec4(albedo * diffuse, 1.0);
}
