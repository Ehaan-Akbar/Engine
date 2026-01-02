#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec4 fragTangent;
layout(location = 4) in vec2 fragUV;

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
    uint albedoIndex;
    uint roughnessIndex;
    uint normalIndex;
    uint occlusionIndex;
    uint emissiveIndex;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

layout(set = 0, binding = 1) buffer ObjectBuffer {
    ObjectSSBO objectSSBOs[];
};

layout(set = 1, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform Push {
    uint uboIndex;
} push;



void main() {
    ObjectSSBO object = objectSSBOs[nonuniformEXT(push.uboIndex)];

    outAlbedo = vec4(texture(textures[nonuniformEXT(object.albedoIndex)], fragUV));

    vec3 normal = texture(textures[nonuniformEXT(object.normalIndex)], fragUV).xyz;
    vec3 bitangent = cross(fragNormal, fragTangent.xyz) * fragTangent.w;
    mat3 TBN = mat3(fragTangent.xyz, bitangent, fragNormal);


    outNormal = vec4(normalize(TBN * normal), 1.0);
    //outNormal = vec4(normalize(fragNormal) * 0.5 + 0.5, 1.0);

    vec4 roughnessMetallic = texture(textures[nonuniformEXT(object.roughnessIndex)], fragUV);
    
    //r channel = metallic
    //g channel = roughness
    outMaterial = vec4(roughnessMetallic.g, roughnessMetallic.b, 0.0 ,0.0);
}