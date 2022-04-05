//------------------------------------------------------------------------------
// Image based lighting configuration
//------------------------------------------------------------------------------

// IBL techniques
#define IBL_TECHNIQUE_INFINITE      0u
#define IBL_TECHNIQUE_FINITE_SPHERE 1u
#define IBL_TECHNIQUE_FINITE_BOX    2u

// Number of spherical harmonics bands (1, 2 or 3)
#define SPHERICAL_HARMONICS_BANDS           3

// IBL integration algorithm
#define IBL_INTEGRATION_PREFILTERED_CUBEMAP         0
#define IBL_INTEGRATION_IMPORTANCE_SAMPLING         1

#define IBL_INTEGRATION                             IBL_INTEGRATION_PREFILTERED_CUBEMAP

#define IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT   64

//------------------------------------------------------------------------------
// IBL utilities
//------------------------------------------------------------------------------

void Swap(inout float a, inout float b)
{
    float tmp = a;
    a = b;
    b = tmp;
}

// Returns the two roots of Ax^2 + Bx + C = 0, assuming that A != 0
// The returned roots (if finite) satisfy roots.x <= roots.y
vec2 SolveQuadratic(float A, float B, float C)
{
    // From Numerical Recipes in C
    float q = -0.5 * (B + sign(B) * sqrt(B * B - 4.0 * A * C));
    vec2 roots = vec2(q / A, C / q);
    if (roots.x > roots.y) Swap(roots.x, roots.y);
    return roots;
}

vec2 IntersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max3(t1);
    float tFar = min3(t2);
    return vec2(tNear, tFar);
}

// Assume: a <= b
float GetSmallestPositive(float a, float b) {
    return a >= 0.0 ? a : b;
}

vec3 decodeDataForIBL(const vec4 data) {
    return data.rgb;
}

//------------------------------------------------------------------------------
// IBL prefiltered DFG term implementations
//------------------------------------------------------------------------------

vec3 PrefilteredDFG_LUT(float lod, float NoV) {
    // coord = sqrt(linear_roughness), which is the mapping used by cmgen.
    return textureLod(light_iblDFG, vec2(NoV, lod), 0.0).rgb;
}

//------------------------------------------------------------------------------
// IBL environment BRDF dispatch
//------------------------------------------------------------------------------

vec3 prefilteredDFG(float perceptualRoughness, float NoV) {
    // PrefilteredDFG_LUT() takes a LOD, which is sqrt(roughness) = perceptualRoughness
    return PrefilteredDFG_LUT(perceptualRoughness, NoV);
}

//------------------------------------------------------------------------------
// IBL irradiance implementations
//------------------------------------------------------------------------------

vec3 Irradiance_SphericalHarmonics(const vec3 n) {
    return max(
          frameUniforms.iblSH[0]
#if SPHERICAL_HARMONICS_BANDS >= 2
        + frameUniforms.iblSH[1] * (n.y)
        + frameUniforms.iblSH[2] * (n.z)
        + frameUniforms.iblSH[3] * (n.x)
#endif
#if SPHERICAL_HARMONICS_BANDS >= 3
        + frameUniforms.iblSH[4] * (n.y * n.x)
        + frameUniforms.iblSH[5] * (n.y * n.z)
        + frameUniforms.iblSH[6] * (3.0 * n.z * n.z - 1.0)
        + frameUniforms.iblSH[7] * (n.z * n.x)
        + frameUniforms.iblSH[8] * (n.x * n.x - n.y * n.y)
#endif
        , 0.0);
}

vec3 Irradiance_RoughnessOne(const vec3 n) {
    // note: lod used is always integer, hopefully the hardware skips tri-linear filtering
    return decodeDataForIBL(textureLod(light_iblSpecular, n, frameUniforms.iblRoughnessOneLevel));
}

//------------------------------------------------------------------------------
// IBL irradiance dispatch
//------------------------------------------------------------------------------

vec3 diffuseIrradiance(const vec3 n) {
    if (frameUniforms.iblSH[0].x == 65504.0) {
#if FILAMENT_QUALITY < FILAMENT_QUALITY_HIGH
        return Irradiance_RoughnessOne(n);
#else
        ivec2 s = textureSize(light_iblSpecular, int(frameUniforms.iblRoughnessOneLevel));
        float du = 1.0 / float(s.x);
        float dv = 1.0 / float(s.y);
        vec3 m0 = normalize(cross(n, vec3(0.0, 1.0, 0.0)));
        vec3 m1 = cross(m0, n);
        vec3 m0du = m0 * du;
        vec3 m1dv = m1 * dv;
        vec3 c;
        c  = Irradiance_RoughnessOne(n - m0du - m1dv);
        c += Irradiance_RoughnessOne(n + m0du - m1dv);
        c += Irradiance_RoughnessOne(n + m0du + m1dv);
        c += Irradiance_RoughnessOne(n - m0du + m1dv);
        return c * 0.25;
#endif
    } else {
        return Irradiance_SphericalHarmonics(n);
    }
}

//------------------------------------------------------------------------------
// IBL specular
//------------------------------------------------------------------------------

// Helper function that converts the incoming Z-up world space reflection vector
// to a Filament IBL texture lookup vector, where the top face is actually +Y.
vec3 zUpToIblDirection(vec3 r) {
    mat3 rotationMat = mat3(frameUniforms.iblRotation);
    r = rotationMat * r;
#if defined(IN_SHAPR_SHADER)
    return vec3(-r.x, r.z, r.y);
#else
    return r;
#endif
}

float OverlayBlend(float a, float b) {
    // Visually almost-luminance-preserving formulation. This uses an artist-provided constant below
    // that they've found the visually most pleasing under _most_ (but not all) circumstances.
    const float kT = 0.65;
    const float kOneOverT = 1.0 / kT;
    return (a < kT) ? a * kOneOverT * b : a - kT + b;
}
vec3 OverlayBlend(vec3 a, vec3 b) {
    return vec3(OverlayBlend(a.x, b.x), OverlayBlend(a.y, b.y), OverlayBlend(a.z, b.z));
}

vec3 TintIbl(vec3 color) {
    return mix(color, OverlayBlend(color, frameUniforms.iblTintAndIntensity.rgb), frameUniforms.iblTintAndIntensity.w);
}

float perceptualRoughnessToLod(float perceptualRoughness) {
    // The mapping below is a quadratic fit for log2(perceptualRoughness)+iblRoughnessOneLevel when
    // iblRoughnessOneLevel is 4. We found empirically that this mapping works very well for
    // a 256 cubemap with 5 levels used. But also scales well for other iblRoughnessOneLevel values.
    return frameUniforms.iblRoughnessOneLevel * perceptualRoughness * (2.0 - perceptualRoughness);
}

vec3 prefilteredRadiance(const vec3 r, float perceptualRoughness) {
    float lod = perceptualRoughnessToLod(perceptualRoughness);
    return TintIbl(decodeDataForIBL(textureLod(light_iblSpecular, zUpToIblDirection(r), lod)));
}

vec3 prefilteredRadiance(const vec3 r, float roughness, float offset) {
    float lod = frameUniforms.iblRoughnessOneLevel * roughness;
    return TintIbl(decodeDataForIBL(textureLod(light_iblSpecular, zUpToIblDirection(r), lod + offset)));
}

vec3 getSpecularDominantDirection(const vec3 n, const vec3 r, float roughness) {
    return mix(r, n, roughness * roughness);
}

vec3 specularDFG(const PixelParams pixel) {
#if defined(SHADING_MODEL_CLOTH)
    return pixel.f0 * pixel.dfg.z;
#else
    return mix(pixel.dfg.xxx, pixel.dfg.yyy, pixel.f0);
#endif
}


/**
 * This function returns an IBL lookup direction, taking into account the current IBL type (e.g. infinite spherical, 
 * finite/local sphere/box), an initial intended lookup direction (baseDir) and the particular normal we compute
 * reflections against (e.g. either the interpolated surface or clearcoat normal).
 */
vec3 GetAdjustedReflectedDirection(const vec3 baseDir, const vec3 normal) {
    vec3 defaultReflected = reflect(-baseDir, normal);    

    if (frameUniforms.iblTechnique == IBL_TECHNIQUE_INFINITE) return defaultReflected;

    // intersect the ray rayPos + t * rayDir with the finite geometry; done in the coordinate system of the finite geometry
    vec3 rayPos = getWorldPosition() + getWorldOffset() - frameUniforms.iblCenter;
    vec3 rayDir = defaultReflected;

    vec3  r  = vec3(0.0); // resulting direction
    float t0 = -1.0f;     // intersection parameter between ray and finite IBL geometry
    
    if (frameUniforms.iblTechnique == IBL_TECHNIQUE_FINITE_SPHERE) {
        float R2 = frameUniforms.iblHalfExtents.r; // we store the squared radius to shave off a multiplication here
        float A = 1.0; // in general, this should be dot(rayDir, rayDir) but we have just normalized it a couple of lines ago
        float B = 2.0 * dot(rayPos, rayDir);
        float C = dot(rayPos, rayPos) - R2;
        vec2 roots = SolveQuadratic(A, B, C);
        t0 = GetSmallestPositive(roots.x, roots.y);
    }
    else if (frameUniforms.iblTechnique == IBL_TECHNIQUE_FINITE_BOX) {
        vec2 roots = IntersectAABB(rayPos, rayDir, -frameUniforms.iblHalfExtents, frameUniforms.iblHalfExtents);
        t0 = GetSmallestPositive(roots.x, roots.y);
    }

    // translate results back to world space
    vec3 intersection_point = ( t0 >= 0.0 ) ? rayPos + t0 * rayDir : defaultReflected;
    r = normalize(intersection_point);

    return r;
}

/**
 * Returns the reflected vector at the current shading point. The reflected vector
 * return by this function might be different from shading_reflected:
 * - For anisotropic material, we bend the reflection vector to simulate
 *   anisotropic indirect lighting
 * - The reflected vector may be modified to point towards the dominant specular
 *   direction to match reference renderings when the roughness increases
 */

vec3 getReflectedVector(const PixelParams pixel, const vec3 v, const vec3 n) {
#if defined(MATERIAL_HAS_ANISOTROPY)
    vec3  anisotropyDirection = pixel.anisotropy >= 0.0 ? pixel.anisotropicB : pixel.anisotropicT;
    vec3  anisotropicTangent  = cross(anisotropyDirection, v);
    vec3  anisotropicNormal   = cross(anisotropicTangent, anisotropyDirection);
    float bendFactor          = abs(pixel.anisotropy) * saturate(5.0 * pixel.perceptualRoughness);
    vec3  bentNormal          = normalize(mix(n, anisotropicNormal, bendFactor));

    vec3 r = GetAdjustedReflectedDirection(v, bentNormal);
#else
    vec3 r = GetAdjustedReflectedDirection(v, n);
#endif
    return r;
}

vec3 getReflectedVector(const PixelParams pixel, const vec3 n) {
#if defined(MATERIAL_HAS_ANISOTROPY)
    vec3 r = getReflectedVector(pixel, shading_view, n);
#else
    vec3 r = GetAdjustedReflectedDirection(shading_view, n);
#endif
    return getSpecularDominantDirection(n, r, pixel.roughness);
}

//------------------------------------------------------------------------------
// Prefiltered importance sampling
//------------------------------------------------------------------------------

#if IBL_INTEGRATION == IBL_INTEGRATION_IMPORTANCE_SAMPLING
vec2 hammersley(uint index) {
    const uint numSamples = uint(IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT);
    const float invNumSamples = 1.0 / float(numSamples);
    const float tof = 0.5 / float(0x80000000U);
    uint bits = index;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return vec2(float(index) * invNumSamples, float(bits) * tof);
}

vec3 importanceSamplingNdfDggx(vec2 u, float roughness) {
    // Importance sampling D_GGX
    float a2 = roughness * roughness;
    float phi = 2.0 * PI * u.x;
    float cosTheta2 = (1.0 - u.y) / (1.0 + (a2 - 1.0) * u.y);
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.0 - cosTheta2);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

vec3 hemisphereCosSample(vec2 u) {
    float phi = 2.0f * PI * u.x;
    float cosTheta2 = 1.0 - u.y;
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.0 - cosTheta2);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

vec3 importanceSamplingVNdfDggx(vec2 u, float roughness, vec3 v) {
    // See: "A Simpler and Exact Sampling Routine for the GGX Distribution of Visible Normals", Eric Heitz
    float alpha = roughness;

    // stretch view
    v = normalize(vec3(alpha * v.x, alpha * v.y, v.z));

    // orthonormal basis
    vec3 up = abs(v.z) < 0.9999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 t = normalize(cross(up, v));
    vec3 b = cross(t, v);

    // sample point with polar coordinates (r, phi)
    float a = 1.0 / (1.0 + v.z);
    float r = sqrt(u.x);
    float phi = (u.y < a) ? u.y / a * PI : PI + (u.y - a) / (1.0 - a) * PI;
    float p1 = r * cos(phi);
    float p2 = r * sin(phi) * ((u.y < a) ? 1.0 : v.z);

    // compute normal
    vec3 h = p1 * t + p2 * b + sqrt(max(0.0, 1.0 - p1*p1 - p2*p2)) * v;

    // unstretch
    h = normalize(vec3(alpha * h.x, alpha * h.y, max(0.0, h.z)));
    return h;
}

float prefilteredImportanceSampling(float ipdf, float omegaP) {
    // See: "Real-time Shading with Filtered Importance Sampling", Jaroslav Krivanek
    // Prefiltering doesn't work with anisotropy
    const float numSamples = float(IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT);
    const float invNumSamples = 1.0 / float(numSamples);
    const float K = 4.0;
    float omegaS = invNumSamples * ipdf;
    float mipLevel = log2(K * omegaS / omegaP) * 0.5;    // log4
    return mipLevel;
}

vec3 isEvaluateSpecularIBL(const MaterialInputs material, const PixelParams pixel, vec3 n, vec3 v, float NoV) {
    const uint numSamples = uint(IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT);
    const float invNumSamples = 1.0 / float(numSamples);
    const vec3 up = vec3(0.0, 0.0, 1.0);

    // TODO: for a true anisotropic BRDF, we need a real tangent space
    // tangent space
    mat3 T;
    T[0] = normalize(cross(up, n));
    T[1] = cross(n, T[0]);
    T[2] = n;

    // Random rotation around N per pixel
    const vec3 m = vec3(0.06711056, 0.00583715, 52.9829189);
    float a = 2.0 * PI * fract(m.z * fract(dot(gl_FragCoord.xy, m.xy)));
    float c = cos(a);
    float s = sin(a);
    mat3 R;
    R[0] = vec3( c, s, 0);
    R[1] = vec3(-s, c, 0);
    R[2] = vec3( 0, 0, 1);
    T *= R;

    float roughness = pixel.roughness;
    float dim = float(textureSize(light_iblSpecular, 0).x);
    float omegaP = (4.0 * PI) / (6.0 * dim * dim);

    vec3 indirectSpecular = vec3(0.0);
    for (uint i = 0u; i < numSamples; i++) {
        vec2 u = hammersley(i);
        vec3 h = T * importanceSamplingNdfDggx(u, roughness);

        // Since anisotropy doesn't work with prefiltering, we use the same "faux" anisotropy
        // we do when we use the prefiltered cubemap
        vec3 l = getReflectedVector(pixel, v, h);

        // Compute this sample's contribution to the brdf
        float NoL = saturate(dot(n, l));
        if (NoL > 0.0) {
            float NoH = dot(n, h);
            float LoH = saturate(dot(l, h));

            // PDF inverse (we must use D_GGX() here, which is used to generate samples)
            float ipdf = (4.0 * LoH) / (D_GGX(roughness, NoH, h) * NoH);
            float mipLevel = prefilteredImportanceSampling(ipdf, omegaP);
            vec3 L = decodeDataForIBL(textureLod(light_iblSpecular, l, mipLevel));

            float D = distribution(roughness, NoH, h);
            float V = visibility(roughness, NoV, NoL);
            vec3 F = material.specularIntensity * fresnel(pixel.f0, LoH);
            vec3 Fr = F * (D * V * NoL * ipdf * invNumSamples);

            indirectSpecular += (Fr * L);
        }
    }

    return indirectSpecular;
}

vec3 isEvaluateDiffuseIBL(const MaterialInputs material, const PixelParams pixel, vec3 n, vec3 v) {
    const uint numSamples = uint(IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT);
    const float invNumSamples = 1.0 / float(numSamples);
    const vec3 up = vec3(0.0, 0.0, 1.0);

    // TODO: for a true anisotropic BRDF, we need a real tangent space
    // tangent space
    mat3 T;
    T[0] = normalize(cross(up, n));
    T[1] = cross(n, T[0]);
    T[2] = n;

    // Random rotation around N per pixel
    const vec3 m = vec3(0.06711056, 0.00583715, 52.9829189);
    float a = 2.0 * PI * fract(m.z * fract(dot(gl_FragCoord.xy, m.xy)));
    float c = cos(a);
    float s = sin(a);
    mat3 R;
    R[0] = vec3( c, s, 0);
    R[1] = vec3(-s, c, 0);
    R[2] = vec3( 0, 0, 1);
    T *= R;

    float dim = float(textureSize(light_iblSpecular, 0).x);
    float omegaP = (4.0 * PI) / (6.0 * dim * dim);

    vec3 indirectDiffuse = vec3(0.0);
    for (uint i = 0u; i < numSamples; i++) {
        vec2 u = hammersley(i);
        vec3 h = T * hemisphereCosSample(u);

        // Since anisotropy doesn't work with prefiltering, we use the same "faux" anisotropy
        // we do when we use the prefiltered cubemap
        vec3 l = getReflectedVector(pixel, v, h);

        // Compute this sample's contribution to the brdf
        float NoL = saturate(dot(n, l));
        if (NoL > 0.0) {
            // PDF inverse (we must use D_GGX() here, which is used to generate samples)
            float ipdf = PI / NoL;
            // we have to bias the mipLevel (+1) to help with very strong highlights
            float mipLevel = prefilteredImportanceSampling(ipdf, omegaP) + 1.0;
            vec3 L = decodeDataForIBL(textureLod(light_iblSpecular, l, mipLevel));
            indirectDiffuse += L;
        }
    }

    return indirectDiffuse * invNumSamples; // we bake 1/PI here, which cancels out
}

void isEvaluateClearCoatIBL(const MaterialInputs material, const PixelParams pixel, float specularAO, inout vec3 Fd, inout vec3 Fr) {
#if defined(MATERIAL_HAS_CLEAR_COAT)
#if defined(MATERIAL_HAS_NORMAL) || defined(MATERIAL_HAS_CLEAR_COAT_NORMAL)
    // We want to use the geometric normal for the clear coat layer
    float clearCoatNoV = clampNoV(dot(shading_clearCoatNormal, shading_view));
    vec3 clearCoatNormal = shading_clearCoatNormal;
#else
    float clearCoatNoV = shading_NoV;
    vec3 clearCoatNormal = shading_normal;
#endif
    // The clear coat layer assumes an IOR of 1.5 (4% reflectance)
    float Fc = F_Schlick(0.04, 1.0, clearCoatNoV) * pixel.clearCoat;
    float attenuation = 1.0 - Fc;
    Fd *= attenuation;
    Fr *= attenuation;

    PixelParams p;
    p.perceptualRoughness = pixel.clearCoatPerceptualRoughness;
    p.f0 = vec3(0.04);
    p.roughness = perceptualRoughnessToRoughness(p.perceptualRoughness);
#if defined(MATERIAL_HAS_ANISOTROPY)
    p.anisotropy = 0.0;
#endif

    vec3 clearCoatLobe = isEvaluateSpecularIBL(material, p, clearCoatNormal, shading_view, clearCoatNoV);
    Fr += clearCoatLobe * (specularAO * pixel.clearCoat);
#endif
}
#endif

//------------------------------------------------------------------------------
// IBL evaluation
//------------------------------------------------------------------------------

void evaluateClothIndirectDiffuseBRDF(const PixelParams pixel, inout float diffuse) {
#if defined(SHADING_MODEL_CLOTH)
#if defined(MATERIAL_HAS_SUBSURFACE_COLOR)
    // Simulate subsurface scattering with a wrap diffuse term
    diffuse *= Fd_Wrap(shading_NoV, 0.5);
#endif
#endif
}

void evaluateSheenIBL(const MaterialInputs material, const PixelParams pixel, float diffuseAO,
        const in SSAOInterpolationCache cache, inout vec3 Fd, inout vec3 Fr) {
#if !defined(SHADING_MODEL_CLOTH) && !defined(SHADING_MODEL_SUBSURFACE)
#if defined(MATERIAL_HAS_SHEEN_COLOR)
    // Albedo scaling of the base layer before we layer sheen on top
    Fd *= pixel.sheenScaling;
    Fr *= pixel.sheenScaling;

    vec3 reflectance = pixel.sheenDFG * pixel.sheenColor;
    reflectance *= specularAO(shading_NoV, diffuseAO, pixel.sheenRoughness, cache);

    vec3 r = GetAdjustedReflectedDirection(shading_view, shading_normal);
    Fr += material.specularIntensity * reflectance * prefilteredRadiance(r, pixel.sheenPerceptualRoughness);
#endif
#endif
}

void evaluateClearCoatIBL(const MaterialInputs material, const PixelParams pixel, float diffuseAO,
        const in SSAOInterpolationCache cache, inout vec3 Fd, inout vec3 Fr) {
#if IBL_INTEGRATION == IBL_INTEGRATION_IMPORTANCE_SAMPLING
    float specularAO = specularAO(shading_NoV, diffuseAO, pixel.clearCoatRoughness, cache);
    isEvaluateClearCoatIBL(material, pixel, specularAO, Fd, Fr);
    return;
#endif

#if defined(MATERIAL_HAS_CLEAR_COAT)
#if defined(MATERIAL_HAS_NORMAL) || defined(MATERIAL_HAS_CLEAR_COAT_NORMAL)
    // We want to use the geometric normal for the clear coat layer
    float clearCoatNoV = clampNoV(dot(shading_clearCoatNormal, shading_view));
    vec3 clearCoatR = GetAdjustedReflectedDirection(shading_view, shading_clearCoatNormal);
#else
    float clearCoatNoV = shading_NoV;
    vec3 clearCoatR = GetAdjustedReflectedDirection(shading_view, shading_normal);
#endif

    // The clear coat layer assumes an IOR of 1.5 (4% reflectance)
    float Fc = F_Schlick(0.04, 1.0, clearCoatNoV) * pixel.clearCoat;
    float attenuation = 1.0 - Fc;
    Fd *= attenuation;
    Fr *= attenuation;

    // TODO: Should we apply specularAO to the attenuation as well?
    float specularAO = specularAO(clearCoatNoV, diffuseAO, pixel.clearCoatRoughness, cache);
    
    Fr += prefilteredRadiance(clearCoatR, pixel.clearCoatPerceptualRoughness) * (specularAO * Fc);
#endif
}

void evaluateSubsurfaceIBL(const PixelParams pixel, const vec3 diffuseIrradiance,
        inout vec3 Fd, inout vec3 Fr) {
#if defined(SHADING_MODEL_SUBSURFACE)
    vec3 viewIndependent = diffuseIrradiance;
    vec3 viewDependent = prefilteredRadiance(-shading_view, pixel.roughness, 1.0 + pixel.thickness);
    float attenuation = (1.0 - pixel.thickness) / (2.0 * PI);
    Fd += pixel.subsurfaceColor * (viewIndependent + viewDependent) * attenuation;
#elif defined(SHADING_MODEL_CLOTH) && defined(MATERIAL_HAS_SUBSURFACE_COLOR)
    Fd *= saturate(pixel.subsurfaceColor + shading_NoV);
#endif
}

#if defined(HAS_REFRACTION)

struct Refraction {
    vec3 position;
    vec3 direction;
    float d;
};

void refractionSolidSphere(const PixelParams pixel,
    const vec3 n, vec3 r, out Refraction ray) {
    r = refract(r, n, pixel.etaIR);
    float NoR = dot(n, r);
    float d = pixel.thickness * -NoR;
    ray.position = vec3(shading_position + r * d);
    ray.d = d;
    vec3 n1 = normalize(NoR * r - n * 0.5);
    ray.direction = refract(r, n1,  pixel.etaRI);
}

void refractionSolidBox(const PixelParams pixel,
    const vec3 n, vec3 r, out Refraction ray) {
    vec3 rr = refract(r, n, pixel.etaIR);
    float NoR = dot(n, rr);
    float d = pixel.thickness / max(-NoR, 0.001);
    ray.position = vec3(shading_position + rr * d);
    ray.direction = r;
    ray.d = d;
#if REFRACTION_MODE == REFRACTION_MODE_CUBEMAP
    // fudge direction vector, so we see the offset due to the thickness of the object
    float envDistance = 10.0; // this should come from a ubo
    ray.direction = normalize((ray.position - shading_position) + ray.direction * envDistance);
#endif
}

void refractionThinSphere(const PixelParams pixel,
    const vec3 n, vec3 r, out Refraction ray) {
    float d = 0.0;
#if defined(MATERIAL_HAS_MICRO_THICKNESS)
    // note: we need the refracted ray to calculate the distance traveled
    // we could use shading_NoV, but we would lose the dependency on ior.
    vec3 rr = refract(r, n, pixel.etaIR);
    float NoR = dot(n, rr);
    d = pixel.uThickness / max(-NoR, 0.001);
    ray.position = vec3(shading_position + rr * d);
#else
    ray.position = vec3(shading_position);
#endif
    ray.direction = r;
    ray.d = d;
}

void applyRefraction(
    const MaterialInputs material, 
    const PixelParams pixel,
    const vec3 n0, vec3 E, vec3 Fd, vec3 Fr,
    inout vec3 color, inout float alpha) {

    Refraction ray;

#if REFRACTION_TYPE == REFRACTION_TYPE_SOLID
    refractionSolidSphere(pixel, n0, -shading_view, ray);
#elif REFRACTION_TYPE == REFRACTION_TYPE_THIN
    refractionThinSphere(pixel, n0, -shading_view, ray);
#else
#error invalid REFRACTION_TYPE
#endif

    // compute transmission T
#if defined(MATERIAL_HAS_ABSORPTION)
#if defined(MATERIAL_HAS_THICKNESS) || defined(MATERIAL_HAS_MICRO_THICKNESS)
    vec3 T = min(vec3(1.0), exp(-pixel.absorption * ray.d));
#else
    vec3 T = 1.0 - pixel.absorption;
#endif
#endif

    // Roughness remapping so that an IOR of 1.0 means no microfacet refraction and an IOR
    // of 1.5 has full microfacet refraction
    float perceptualRoughness = mix(pixel.perceptualRoughnessUnclamped, 0.0,
            saturate(pixel.etaIR * 3.0 - 2.0));
#if REFRACTION_TYPE == REFRACTION_TYPE_THIN
    // For thin surfaces, the light will bounce off at the second interface in the direction of
    // the reflection, effectively adding to the specular, but this process will repeat itself.
    // Each time the ray exits the surface on the front side after the first bounce,
    // it's multiplied by E^2, and we get: E + E(1-E)^2 + E^3(1-E)^2 + ...
    // This infinite series converges and is easy to simplify.
    // Note: we calculate these bounces only on a single component,
    // since it's a fairly subtle effect.
    E *= 1.0 + pixel.transmission * (1.0 - E.g) / (1.0 + E.g);
#endif

    /* sample the cubemap or screen-space */
#if REFRACTION_MODE == REFRACTION_MODE_CUBEMAP
    // when reading from the cubemap, we are not pre-exposed so we apply iblLuminance
    // which is not the case when we'll read from the screen-space buffer
    vec3 Ft = prefilteredRadiance(ray.direction, perceptualRoughness) * frameUniforms.iblLuminance;
    float at = 1.0;
#else
    // compute the point where the ray exits the medium, if needed
    vec4 p = vec4(frameUniforms.clipFromWorldMatrix * vec4(ray.position, 1.0));
    p.xy = uvToRenderTargetUV(p.xy * (0.5 / p.w) + 0.5);

    // perceptualRoughness to LOD
    // Empirical factor to compensate for the gaussian approximation of Dggx, chosen so
    // cubemap and screen-space modes match at perceptualRoughness 0.125
    // TODO: Remove this factor temporarily until we find a better solution
    //       This overblurs many scenes and needs a more principled approach
    // float tweakedPerceptualRoughness = perceptualRoughness * 1.74;
    float tweakedPerceptualRoughness = perceptualRoughness;
    float lod = max(0.0, 2.0 * log2(tweakedPerceptualRoughness) + frameUniforms.refractionLodOffset);

    vec4 Fat = textureLod(light_ssr, p.xy, lod);
    vec3 Ft = Fat.rgb;
    float at = Fat.a;
#endif

    // base color changes the amount of light passing through the boundary
    Ft *= pixel.diffuseColor;

    // fresnel from the first interface
    Ft *= 1.0 - E;
    at += (1.0 - at) * max3(E);

    // apply absorption
#if defined(MATERIAL_HAS_ABSORPTION)
    Ft *= T;
#endif

    Fr *= material.specularIntensity * frameUniforms.iblLuminance;
    Fd *= frameUniforms.iblLuminance;
    color += Fr + mix(Fd, Ft, pixel.transmission);
    alpha *= mix(1.0, at, pixel.transmission);
}
#endif

void combineDiffuseAndSpecular(
        const MaterialInputs material, 
        const PixelParams pixel,
        const vec3 n, const vec3 E, const vec3 Fd, const vec3 Fr,
        inout vec3 color, inout float alpha) {
#if defined(HAS_REFRACTION)
    applyRefraction(material, pixel, n, E, Fd, Fr, color, alpha);
#else
    color += (Fd + material.specularIntensity * Fr) * frameUniforms.iblLuminance;
#endif
}

void evaluateIBL(const MaterialInputs material, const PixelParams pixel, inout vec3 color, inout float alpha) {
    // specular layer
    vec3 Fr;
#if IBL_INTEGRATION == IBL_INTEGRATION_PREFILTERED_CUBEMAP
    vec3 E = specularDFG(pixel);
    // we have to modify the IBL specular evaluation direction for anisotropic materials
    vec3 r = getReflectedVector(pixel, shading_view, shading_normal);

    Fr = E * prefilteredRadiance(r, pixel.perceptualRoughness);
#elif IBL_INTEGRATION == IBL_INTEGRATION_IMPORTANCE_SAMPLING
    vec3 E = vec3(0.0); // TODO: fix for importance sampling
    Fr = isEvaluateSpecularIBL(material, pixel, shading_normal, shading_view, shading_NoV);
#endif

    SSAOInterpolationCache interpolationCache;
#if defined(BLEND_MODE_OPAQUE) || defined(BLEND_MODE_MASKED)
    interpolationCache.uv = uvToRenderTargetUV(getNormalizedViewportCoord().xy);
#endif

    float ssao = evaluateSSAO(interpolationCache);
    float diffuseAO = min(material.ambientOcclusion, ssao);
    float specularAO = specularAO(shading_NoV, diffuseAO, pixel.roughness, interpolationCache);

    Fr *= singleBounceAO(specularAO) * pixel.energyCompensation;

    // diffuse layer
    float diffuseBRDF = singleBounceAO(diffuseAO); // Fd_Lambert() is baked in the SH below
    evaluateClothIndirectDiffuseBRDF(pixel, diffuseBRDF);

#if defined(MATERIAL_HAS_BENT_NORMAL)
    vec3 diffuseNormal = shading_bentNormal;
#else
    vec3 diffuseNormal = shading_normal;
#endif

#if IBL_INTEGRATION == IBL_INTEGRATION_PREFILTERED_CUBEMAP
    vec3 diffuseIrradiance = diffuseIrradiance(diffuseNormal);
#elif IBL_INTEGRATION == IBL_INTEGRATION_IMPORTANCE_SAMPLING
    vec3 diffuseIrradiance = isEvaluateDiffuseIBL(pixel, diffuseNormal, shading_view);
#endif
    vec3 Fd = pixel.diffuseColor * diffuseIrradiance * (1.0 - E) * diffuseBRDF;

    // subsurface layer
    evaluateSubsurfaceIBL(pixel, diffuseIrradiance, Fd, Fr);

    // extra ambient occlusion term for the base and subsurface layers
    multiBounceAO(diffuseAO, pixel.diffuseColor, Fd);
    multiBounceSpecularAO(specularAO, pixel.f0, Fr);

    // sheen layer
    evaluateSheenIBL(material, pixel, diffuseAO, interpolationCache, Fd, Fr);

    // clear coat layer
    evaluateClearCoatIBL(material, pixel, diffuseAO, interpolationCache, Fd, Fr);

    // Note: iblLuminance is already premultiplied by the exposure
    combineDiffuseAndSpecular(material, pixel, shading_normal, E, Fd, Fr, color, alpha);
}
