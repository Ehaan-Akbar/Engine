#version 450

vec2 positions[6] = vec2[](
	vec2(-1.0, -1.0),
	vec2(1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, -1.0),
	vec2(1.0, 1.0),
	vec2(-1.0, 1.0)
);

vec2 uvs[6] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 0.0),
	vec2(1.0, 1.0),
	vec2(0.0, 1.0)
);

//set 0 - Global
//set 1 - Resources
//set 2 - Target

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec4 camPos;
    vec4 dimensions;
    mat4 inverseProjection;
    mat4 inverseView;
	vec4 numOfEntities;
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
layout(set = 2, binding = 3) uniform sampler2D depthImage;

layout(push_constant) uniform Push {
    uint uboIndex;
    uint textureIndex;
} push;

layout(location = 0) out vec2 fragUV;

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragUV = uvs[gl_VertexIndex];
}