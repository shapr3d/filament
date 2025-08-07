#if defined(TARGET_MOBILE)
    // min roughness such that (MIN_PERCEPTUAL_ROUGHNESS^4) > 0 in fp16 (i.e. 2^(-14/4), rounded up)
    #define MIN_PERCEPTUAL_ROUGHNESS 0.089
    #define MIN_ROUGHNESS            0.007921
#else
    #define MIN_PERCEPTUAL_ROUGHNESS 0.045
    #define MIN_ROUGHNESS            0.002025
#endif

#define MIN_N_DOT_V 1e-4

float clampNoV(float NoV) {
    // Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
    return max(NoV, MIN_N_DOT_V);
}

vec3 computeDiffuseColor(const vec4 baseColor, float metallic) {
    return baseColor.rgb * (1.0 - metallic);
}

vec3 computeF0(const vec4 baseColor, float metallic, float reflectance) {
    return baseColor.rgb * metallic + (reflectance * (1.0 - metallic));
}

float F0scalar(float n, float k) {
    float n1 = n - 1.0, n2 = n + 1.0;
    return (n1*n1 + k*k) / (n2*n2 + k*k); 
}

// ---------- Fade factor: 1 at glossy … 0 at very rough ----------
float iorFade(float perceptualRoughness) {
    return 1.0 - smoothstep(0.95, 1.0, perceptualRoughness);
}

vec3 computeF0(const vec4  baseColor,
               float       metallic,
               float       reflectance,      // 0‑1 slider for plastics
               float       nd, float k,      // complex IOR
               float       perceptualRoughness)
{
    // -- Dielectric branch (never touches nd / k) -------------------------
    float  F0_dielectric = 0.16 * reflectance * reflectance;

    F0_dielectric = reflectance;
    // -- Metal branch with IOR + fade -------------------------------------
    //loat  F0_phys   = F0scalar(nd, k) / F0scalar(1.5, 0.0);          // R₀ from (n,k)
    float  F0_phys   = F0scalar(nd, k);
    float  F0_mix    = mix(1.0, F0_phys, iorFade(perceptualRoughness));
    vec3   F0_metal  = baseColor.rgb * F0_mix;   // tint * Fresnel

    return mix(vec3(F0_dielectric), F0_metal, metallic);
}

float computeDielectricF0(float reflectance) {
    return 0.16 * reflectance * reflectance;
}

float computeMetallicFromSpecularColor(const vec3 specularColor) {
    return max3(specularColor);
}

float computeRoughnessFromGlossiness(float glossiness) {
    return 1.0 - glossiness;
}

float perceptualRoughnessToRoughness(float perceptualRoughness) {
    return perceptualRoughness * perceptualRoughness;
}

float roughnessToPerceptualRoughness(float roughness) {
    return sqrt(roughness);
}

float iorToF0(float transmittedIor, float incidentIor) {
    return sq((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

float f0ToIor(float f0) {
    float r = sqrt(f0);
    return (1.0 + r) / (1.0 - r);
}

vec3 f0ClearCoatToSurface(const vec3 f0) {
    // Approximation of iorTof0(f0ToIor(f0), 1.5)
    // This assumes that the clear coat layer has an IOR of 1.5
#if FILAMENT_QUALITY == FILAMENT_QUALITY_LOW
    return saturate(f0 * (f0 * 0.526868 + 0.529324) - 0.0482256);
#else
    return saturate(f0 * (f0 * (0.941892 - 0.263008 * f0) + 0.346479) - 0.0285998);
#endif
}
