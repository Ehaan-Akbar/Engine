#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec3 viewDirection;

//set 0 - Global
//set 1 - Resources

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec4 camPos;
} globalUbo;

layout(set = 1, binding = 1) uniform samplerCube skyboxSamplers[];

layout(push_constant) uniform Push {
    uint uboIndex;
    uint skyboxIndex;
} push;

const vec3 positions[36] = vec3[](
    vec3(-1, -1, -1), vec3(-1,  1, -1), vec3( 1,  1, -1),
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1),

    vec3(-1,  1,  1), vec3(-1,  1, -1), vec3(-1, -1, -1),
    vec3(-1, -1, -1), vec3(-1, -1,  1), vec3(-1,  1,  1),

    vec3( 1,  1, -1), vec3( 1,  1,  1), vec3( 1, -1,  1),
    vec3( 1, -1,  1), vec3( 1, -1, -1), vec3( 1,  1, -1),

    vec3(-1,  1,  1), vec3(-1, -1,  1), vec3( 1, -1,  1),
    vec3( 1, -1,  1), vec3( 1,  1,  1), vec3(-1,  1,  1),

    vec3(-1, -1, -1), vec3( 1, -1, -1), vec3( 1, -1,  1),
    vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, -1, -1),

    vec3(-1,  1, -1), vec3( 1,  1, -1), vec3( 1,  1,  1),
    vec3( 1,  1,  1), vec3(-1,  1,  1), vec3(-1,  1, -1)
);



void main() {
    vec3 pos = positions[gl_VertexIndex];
    viewDirection = pos;
    
    mat4 viewNoTranslation = mat4(mat3(globalUbo.view));

    vec4 clipPos = globalUbo.projection * viewNoTranslation * vec4(pos, 1.0);
    gl_Position = clipPos.xyzw;
}