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

//set 0 - Target

layout(set = 0, binding = 3) uniform sampler2D lightingImage;

layout(push_constant) uniform Push {
    uint uboIndex;
    uint textureIndex;
} push;

layout(location = 0) out vec2 fragUV;

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragUV = uvs[gl_VertexIndex];
}