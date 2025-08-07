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

float  F0scalar(float n, float k) {
    float n1 = n - 1.0, n2 = n + 1.0;
    float num = n1 * n1 + k * k;
    float den = n2 * n2 + k * k;
    return num / den;                                   // 0‒1, never negative
}

vec3 computeF0(const vec4  baseColor,    // RGB = albedo for metals
               float       metallic,     // 0 → dielectric, 1 → metal
               float       reflectance,  // Filament’s scalar slider (0‒1)
               float       nd,           // real part of IOR  (per‑material)
               float       k,            // imaginary part (0 for plastics)
               float       perceptualRoughness)            
{
    // --- dielectric branch -----------------------------------------------
    // Filament maps   Rslider ∈[0,1] ⇒ F0 = 0.16 * Rslider²  (Burley 2012)
    float F0_dielectric = 0.16 * reflectance * reflectance;
    float F0_lambert = 1.0; 
    // --- conductor branch -------------------------------------------------
    float F0_conductor  = F0scalar(nd, k);     // physical formula above

    float iorFade = 1.0 - smoothstep(0.95, 1.0, perceptualRoughness);
    float F0_mix = mix(F0_lambert, F0_conductor, iorFade);
    vec3 F0_metal = baseColor.rgb * F0_mix;

    // --- final blend ------------------------------------------------------
    // metallic==0 ⇒ pure dielectric; metallic==1 ⇒ pure conductor
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
