#version 450
#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec4 fragTangent;
layout(location = 4) out vec2 fragUV;

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

    mat4 model = objectSSBOs[nonuniformEXT(push.uboIndex)].model;
    mat4 view = globalUbo.view;
    mat4 projection = globalUbo.projection;



    vec4 positionWorld = model * vec4(inPosition, 1.0);
    gl_Position = projection * view * positionWorld;
    
    mat3 trans = mat3(transpose(inverse(model)));
    fragNormal = normalize(trans * inNormal);
    fragTangent = vec4(normalize(trans * inTangent.xyz), inTangent.w);

    fragPosWorld = positionWorld.xyz;

    fragColor = inColor;
    fragUV = inUV;
    
}