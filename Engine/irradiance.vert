#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 1) uniform samplerCube skyboxSamplers[];

layout(push_constant) uniform Push {
    uint faceIndex;
    uint mipLevel;
} push;


layout(location = 0) out vec3 fragDir;

const vec2 positions[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

const int indices[6] = int[](0, 1, 2, 2, 1, 3);

const vec3 faceForward[6] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(-1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, -1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, -1.0)
);

const vec3 faceUp[6] = vec3[](
    -vec3(0.0, 1.0, 0.0),
    -vec3(0.0, 1.0, 0.0),
    -vec3(0.0, 0.0, -1.0),
    -vec3(0.0, 0.0, 1.0),
    -vec3(0.0, 1.0, 0.0),
    -vec3(0.0, 1.0, 0.0)
);

void main() {
    int vertexID = indices[gl_VertexIndex];
    vec2 inPos = positions[vertexID];

    gl_Position = vec4(inPos, 0.0, 1.0);

    vec3 forward = faceForward[push.faceIndex];
    vec3 up = faceUp[push.faceIndex];
    vec3 right = normalize(cross(forward, up));

    fragDir = normalize(forward + inPos.x * right + inPos.y * up);
}