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
    vec4 camPos;
    vec4 dimensions;
    mat4 inverseProjection;
    mat4 inverseView;
    vec4 numOfEntities;
} globalUbo;

struct ObjectSSBO {
    mat4 model;
};

//readonly??
layout(set = 0, binding = 1) buffer ObjectBuffer {
    ObjectSSBO objectSSBOs[];
};

struct LightSSBO {
    vec4 type;
    vec4 direction;
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 2) buffer LightBuffer {
    LightSSBO lightSSBOs[];
};



layout(set = 2, binding = 0) uniform sampler2D albedoImage;
layout(set = 2, binding = 1) uniform sampler2D normalImage;
layout(set = 2, binding = 2) uniform sampler2D materialImage;
layout(set = 2, binding = 4) uniform sampler2D depthImage;

layout(push_constant) uniform Push {
    uint uboIndex;
    uint textureIndex;
} push;

vec3 albedo;
vec3 normal;
vec4 material;

vec2 uv;

vec4 clipPos;
vec4 viewPos;
vec4 worldPos;
vec3 fragPos;
vec3 camPos;

float diffuseStrength;
float specularStrength;
float shiny;

struct LightResult {
    vec3 diffuse;
    vec3 specular;
};

LightResult directionalLight(LightSSBO light) {
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    vec3 diffuse = max(dot(normal, lightDir), 0.0) * light.color.xyz;

    vec3 viewDir = normalize(camPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 specular = pow(max(dot(viewDir, reflectDir), 0.0), shiny) * light.color.xyz;

    return LightResult(diffuse, specular);
}

LightResult pointLight(LightSSBO light) {
    return LightResult(vec3(0.0f, 0.0f, 0.0f),vec3(0.0f, 0.0f, 0.0f));
}

LightResult spotLight(LightSSBO light) {
    return LightResult(vec3(0.0f, 0.0f, 0.0f),vec3(0.0f, 0.0f, 0.0f));
}

LightResult areaLight(LightSSBO light) {
    return LightResult(vec3(0.0f, 0.0f, 0.0f),vec3(0.0f, 0.0f, 0.0f));
}

void main() {

    uv = gl_FragCoord.xy / globalUbo.dimensions.xy;
    float depth = texture(depthImage, uv).r;
    float z_ndc = depth;

    clipPos;
    clipPos.xy = uv * 2.0 - 1.0;
    clipPos.z = z_ndc;
    clipPos.w = 1.0;

    viewPos = globalUbo.inverseProjection * clipPos;
    viewPos /= viewPos.w;

    worldPos = globalUbo.inverseView * viewPos;

    albedo = texture(
        albedoImage,
        fragUV
    ).rgb;

    normal = texture(
        normalImage,
        fragUV
    ).xyz * 2.0 - 1.0; // decode if stored in [0,1]

    material = texture(
        materialImage,
        fragUV
    );
    
    normal = normalize(normal);

    fragPos = worldPos.xyz;

    camPos = globalUbo.camPos.xyz;





    float ambient = 0.1f;

    diffuseStrength = 1.0f;
    vec3 diffuse = vec3(0.0f, 0.0f, 0.0f);

    specularStrength = 1.0ff;
    shiny = 16.0f;
    vec3 specular = vec3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < globalUbo.numOfEntities.y; ++i) {
        LightSSBO light = lightSSBOs[i];
        LightResult lighting;

        if (light.type.x == 0) {
            lighting = directionalLight(light);
        } else if (light.type.x == 1) {
            lighting = pointLight(light);
        } else if (light.type.x == 2) {
            lighting = spotLight(light);
        } else if (light.type.x == 3) {
            lighting = areaLight(light);
        }

        diffuse += lighting.diffuse;
        specular += lighting.specular;

    }


    outColor = vec4(albedo * (ambient + diffuseStrength*diffuse + specularStrength*specular), 1.0);
}


