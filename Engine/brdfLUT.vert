#version 450
#extension GL_EXT_nonuniform_qualifier : require

const vec2 positions[4] = vec2[](
	vec2(-1.0, -1.0),
	vec2(1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0)
);

const int indices[6] = int[](0, 1, 2, 2, 1, 3);

void main() {
	int vertexID = indices[gl_VertexIndex];
	vec2 pos = positions[vertexID];

	gl_Position = vec4(pos, 0.0, 1.0);
}