#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

//set 0 - Global
//set 1 - Resources
//set 2 - Target

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec4 lightPos;
    vec4 lightDir;
    vec4 camPos;
    vec4 dimensions;
    mat4 inverseProjection;
    mat4 inverseView;
} globalUbo;

struct ObjectSSBO {
    mat4 model;
};

layout(set = 0, binding = 1) buffer ObjectBuffer {
    ObjectSSBO objectSSBOs[];
};

layout(set = 2, binding = 0) uniform sampler2D albedoImage;
layout(set = 2, binding = 1) uniform sampler2D normalImage;
layout(set = 2, binding = 2) uniform sampler2D materialImage;
layout(set = 2, binding = 4) uniform sampler2D depthImage;

layout(push_constant) uniform Push {
    uint uboIndex;
    uint textureIndex;
} push;


void main() {

    vec2 uv = gl_FragCoord.xy / globalUbo.dimensions.xy;
    float depth = texture(depthImage, uv).r;
    float z_ndc = depth;

    vec4 clipPos;
    clipPos.xy = uv * 2.0 - 1.0;
    clipPos.z = z_ndc;
    clipPos.w = 1.0;

    vec4 viewPos = globalUbo.inverseProjection * clipPos;
    viewPos /= viewPos.w;

    vec4 worldPos = globalUbo.inverseView * viewPos;

    vec3 albedo = texture(
        albedoImage,
        fragUV
    ).rgb;

    vec3 normal = texture(
        normalImage,
        fragUV
    ).xyz * 2.0 - 1.0; // decode if stored in [0,1]

    vec4 material = texture(
        materialImage,
        fragUV
    );



    normal = normalize(normal);

    vec3 lightPos = globalUbo.lightPos.xyz;

    vec3 fragPos = worldPos.xyz;

    vec3 lightDir = normalize(lightPos - fragPos);

    // Diffuse lighting
    float diffuse = max(dot(normal, lightDir), 0.0);

    outColor = vec4(albedo * diffuse, 1.0);
}
