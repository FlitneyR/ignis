
#define PI 3.141592654

float normalDistribution(vec3 halfWay, vec3 normal, float roughness) {
    float r = roughness * roughness;
    float r2 = r * r;
    float NdotH = max(dot(normal, halfWay), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (r2 - 1.0) + 1.0;
    denom *= PI * denom;
    return r2 / denom;
}

float schlickGGX(vec3 normal, vec3 dir, float roughnessProxy) {
    float NdotD = max(dot(normal, dir), 0.0);
    float denom = NdotD * (1 - roughnessProxy) + roughnessProxy;

    return NdotD / denom;
}

float smithSchlickGGX(vec3 normal, vec3 viewDir, vec3 lightDir, float roughnessProxy) {
    return schlickGGX(normal, viewDir, roughnessProxy) * schlickGGX(normal, lightDir, roughnessProxy);
}

vec3 schlickFresnel(vec3 halfway, vec3 viewDir, vec3 ior) {
    float HdotV = max(dot(halfway, viewDir), 0);
    return ior + (1.0 - ior) * pow(1.0 - HdotV, 5.0);
}

float directRougnessProxy(float roughness) {
    roughness += 1;
    return roughness * roughness / 8.0;
}

float iblRoughnessProxy(float roughness) {
    return roughness * roughness / 2.0;
}

vec3 cookTorranceBRDF(vec3 albedo, vec3 normal, vec3 viewDir, vec3 radiance, vec3 lightDir, float metallic, float roughness, float ao) {
    albedo = clamp(albedo, vec3(0), vec3(1));
    radiance = max(vec3(0), radiance);

    normal = normalize(normal);
    viewDir = normalize(viewDir);
    lightDir = normalize(lightDir);

    ao = clamp(ao, 0, 1);
    metallic = clamp(metallic, 0, 1);
    roughness = clamp(roughness, 0.1, 1);

    vec3 lambertionDiffuse = albedo / PI;

    vec3 halfway = normalize(lightDir + viewDir);
    vec3 ior = mix(vec3(0.04), albedo, metallic);
    float roughnessProxy = directRougnessProxy(roughness);

    vec3 fresnel = schlickFresnel(viewDir, halfway, ior);
    vec3 specular = fresnel;
    vec3 diffuse = (vec3(1.0) - specular) * (1.0 - metallic);

    vec3 cookTorranceSpecular = fresnel;
    cookTorranceSpecular *= normalDistribution(halfway, normal, roughness);
    cookTorranceSpecular *= smithSchlickGGX(normal, viewDir, lightDir, roughnessProxy);

    float cookTorranceSpecularDenom = max(4 * dot(viewDir, normal) * dot(lightDir, normal), 0.000001);
    cookTorranceSpecular /= cookTorranceSpecularDenom;

    float NdotL = max(dot(normal, lightDir), 0.000001);
    return (ao * diffuse * lambertionDiffuse + specular * cookTorranceSpecular) * radiance * NdotL;
}
