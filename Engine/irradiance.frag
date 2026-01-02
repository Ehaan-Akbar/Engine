#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 1) uniform samplerCube skyboxSamplers[];

layout(push_constant) uniform Push {
    uint faceIndex;
    uint mipLevel;
} push;

layout(location = 0) in vec3 fragDir;
layout(location = 0) out vec4 outColor;


const float PI = 3.14159265359;
const uint SAMPLE_COUNT = 1024;

//TODO: DO Lambertion convolution, industry standard

vec3 integrateIrradiance(vec3 normal) {
    vec3 irradiance = vec3(0.0);

    for (uint i = 0; i < SAMPLE_COUNT; ++i) {
        float u = float(i) / float(SAMPLE_COUNT);
        float v = fract(sin(float(i) * 12.9898) * 43758.5453);

        float theta = acos(sqrt(1.0 - u));
        float phi = 2.0 * PI * v;

        vec3 sampleDir;
        sampleDir.x = sin(theta) * cos(phi);
        sampleDir.y = sin(theta) * sin(phi);
        sampleDir.z = cos(theta);

        vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
        vec3 tangent = normalize(cross(up, normal));
        vec3 bitangent = cross(normal, tangent);

        vec3 worldSample = tangent * sampleDir.x + bitangent * sampleDir.y + normal * sampleDir.z;
        irradiance += texture(skyboxSamplers[0], worldSample).rgb * cos(theta) * sin(theta);
    }

    irradiance *= PI / float(SAMPLE_COUNT);
    return irradiance;
}

void main() {
    vec3 irradiance = integrateIrradiance(normalize(fragDir));
    outColor = vec4(irradiance, 1.0);
}