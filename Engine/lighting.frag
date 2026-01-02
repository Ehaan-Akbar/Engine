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
    uint albedoIndex;
    uint roughnessIndex;
    uint normalIndex;
    uint occlusionIndex;
    uint emissiveIndex;
    uint _pad0;
    uint _pad1;
    uint _pad2;
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
layout(set = 2, binding = 5) uniform samplerCube irradianceCubeMap;
layout(set = 2, binding = 6) uniform samplerCube prefilterCubeMap;
layout(set = 2, binding = 7) uniform sampler2D brdfLUTMap;

layout(push_constant) uniform Push {
    uint uboIndex;
} push;

const float PI = 3.141595359;

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
    vec3 lightDir = light.direction.xyz;
    vec3 diffuse = max(dot(normal, lightDir), 0.0) * light.color.xyz;

    vec3 viewDir = normalize(camPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 specular = pow(max(dot(viewDir, reflectDir), 0.0), shiny) * light.color.xyz;

    return LightResult(diffuse, specular);
}

LightResult pointLight(LightSSBO light) {
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    vec3 diffuse = max(dot(normal, lightDir), 0.0) * light.color.xyz;

    vec3 viewDir = normalize(camPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 specular = pow(max(dot(viewDir, reflectDir), 0.0), shiny) * light.color.xyz;

    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;

    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);

    return LightResult(diffuse * attenuation, specular * attenuation);
}

LightResult spotLight(LightSSBO light) {
    return LightResult(vec3(0.0f, 0.0f, 0.0f),vec3(0.0f, 0.0f, 0.0f));
}

LightResult areaLight(LightSSBO light) {
    return LightResult(vec3(0.0f, 0.0f, 0.0f),vec3(0.0f, 0.0f, 0.0f));
}


vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return a2 / max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggxV = GeometrySchlickGGX(NdotV, roughness);
    float ggxL = GeometrySchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

vec3 calculatePBR(vec3 N, vec3 V, vec3 fragPos, vec3 albedo, float metallic, float roughness, LightSSBO light) {
    vec3 L = normalize(light.position.xyz - fragPos);
    vec3 H = normalize(V + L);
    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0 / (0.09 * distance * distance);
    vec3 radiance = light.color.xyz * attenuation;

    //Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denominator;

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    return Lo;
}

void main() {

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
    
    float roughness = material.r;
    float metallic = material.g;
    


    uv = gl_FragCoord.xy / globalUbo.dimensions.xy;
    float depth = texture(depthImage, uv).r;
    if (depth >= 1.0) {
        discard;
    }
    float z_ndc = depth;

    clipPos;
    clipPos.xy = uv * 2.0 - 1.0;
    clipPos.z = z_ndc;
    clipPos.w = 1.0;

    viewPos = globalUbo.inverseProjection * clipPos;
    viewPos /= viewPos.w;

    worldPos = globalUbo.inverseView * viewPos;
    
    normal = normalize(normal);

    fragPos = worldPos.xyz;

    camPos = globalUbo.camPos.xyz;





   /*float ambient = 0.0f;

    diffuseStrength = 0.5f;
    vec3 diffuse = vec3(0.0f, 0.0f, 0.0f);

    specularStrength = 1.0f;
    shiny = 16.0;
    //shiny = 16.0f;
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

    }*/


    vec3 V = normalize(globalUbo.camPos.xyz - fragPos);
    vec3 color = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < globalUbo.numOfEntities.y; ++i) {
        LightSSBO light = lightSSBOs[i];

        

        color += calculatePBR(normal, V, fragPos, albedo, metallic, roughness, light);
    }

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlickRoughness(max(dot(normal, V), 0.0), F0, roughness);

    vec3 irradiance = texture(irradianceCubeMap, normal).rgb;
    vec3 diffuseIBL = irradiance * albedo;

    vec3 kD = (1.0 - F) * (1.0 - metallic);
    diffuseIBL *= kD;

    vec3 R = reflect(-V, normal);
    vec3 prefilteredColor = textureLod(prefilterCubeMap, R, roughness * 4).rgb;
    vec2 brdf = texture(brdfLUTMap, vec2(max(dot(normal, V), 0.0), roughness)).rg;
    vec3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);
    //float specularStrength = 1.0 - roughness;
    //specularIBL *= specularStrength;

    color += diffuseIBL + specularIBL;
    


    //outColor = vec4(albedo * (ambient + diffuseStrength*diffuse + specularStrength*specular), 1.0);
    outColor = vec4(color, 1.0);
    //outColor = vec4(vec3(roughness), 1.0);
}


