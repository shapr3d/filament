//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions for Visualization material implementations
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if (defined(BLEND_MODE_OPAQUE) || defined(BLEND_MODE_MASKED))
#define BLENDING_DISABLED
#else
#define BLENDING_ENABLED
#endif

float SignNoZero(float f) {
    return f < 0.0 ? -1.0 : 1.0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Input adjustment functions
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct FragmentData {
    highp vec3 pos; // world space
    highp vec3 normal; // tangent space
};
const FragmentData FRAGMENT_DATA_INITIAL = FragmentData(vec3(0.0), vec3(0.0));

void ApplyEditorScalers(inout MaterialInputs material) {
#if !defined(IN_SHAPR_SHADER)
    // Temporarily disabling these - we will only need these again when we start authoring mesh environments
    //material.specularScale = 1.0 + materialParams.scalingControl.x;
    //material.diffuseScale = 1.0 + materialParams.scalingControl.z;
#endif
}

FragmentData GetPositionAndNormal() {
    FragmentData result = FRAGMENT_DATA_INITIAL;
#if defined(SHAPR_OBJECT_SPACE_TRILINEAR)
    result.pos = variable_objectSpacePosition.xyz;
#else
    result.pos = getWorldPosition() + getWorldOffset();
#endif
#if 1
#if defined(SHAPR_OBJECT_SPACE_TRILINEAR)
    result.normal = variable_objectSpaceNormal.xyz;
#else
    result.normal = getWorldGeometricNormalVector();
#endif
#else
    // Since I have to re-add this so many times to debug issues, I'm leaving this commented out for the time being
    // Still, be careful: OGL did not expose the fine gradients until 4.5 (!!!) core, so you cannot know if these are
    // coarse (=constant for the 2x2 fragment quad) or fine (forward/backward difference'd) derivatives
    vec3 DxPos = dFdx(result.pos);
    vec3 DyPos = dFdy(result.pos);
    result.normal = normalize(cross(DxPos, DyPos));
#endif
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Blending related functions that implement biplanar texture and normal blending
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct BiplanarAxes {
    ivec3 maximum;
    ivec3 median;
};

struct BiplanarData {
    vec2 maxPos;
    vec2 maxDpDx;
    vec2 maxDpDy;
    float mainWeight;

    vec2 medPos;
    vec2 medDpDx;
    vec2 medDpDy;
    float medianWeight;
};
const BiplanarData DEFAULT_BIPLANAR_DATA = BiplanarData(vec2(0.0), vec2(0.0), vec2(0.0), 0.0, vec2(0.0), vec2(0.0), vec2(0.0), 0.0);

// Selects the two most relevant planes, i.e., where the projection of a plane with the supplied normal would be the largest. The .x integer
// contains the index of the plane with the largest projection (X = 0, Y = 1, Z = 2), whereas .yz are used to project 3D entities to the plane
// that has the normal corresponding (=the same or opposite) to the .x axis. 
BiplanarAxes ComputeBiplanarPlanes(vec3 weights) {
    // This is from https://www.shadertoy.com/view/ws3Bzf. 'Major axis (in x; yz are following axis)'
    ivec3 ma = (weights.x > weights.y && weights.x > weights.z) ? ivec3(0,1,2) :
               (weights.y > weights.z)                          ? ivec3(1,2,0) :
                                                                  ivec3(2,0,1);
    // 'minor axis (in x; yz are following axis)'
    ivec3 mi = (weights.x < weights.y && weights.x < weights.z) ? ivec3(0,1,2) :
               (weights.y < weights.z)                          ? ivec3(1,2,0) :
                                                                  ivec3(2,0,1);
        
    // 'median axis (in x;  yz are following axis)'
    ivec3 me = ivec3(3) - mi - ma;

    BiplanarAxes result;
    result.maximum = ma;
    result.median = me;
    return result;
}

BiplanarData GenerateBiplanarData(BiplanarAxes axes, float scaler, highp vec3 pos, lowp vec3 weights) {
    // Depending on the resolution of the texture, we may want to multiply the texture coordinates
    vec3 queryPos = scaler * pos;

    // Store the query data
    BiplanarData result = DEFAULT_BIPLANAR_DATA;

    // Position needs some fixed flipping-fu so that textures are oriented as intended
    vec2 uvQueries[3] = vec2[3](queryPos.yz * vec2(1, -1), -queryPos.xz, queryPos.yx);

    result.maxPos = uvQueries[axes.maximum.x];
    result.medPos = uvQueries[axes.median.x];

    // Derivatives are based on the scaled pos, without the flipping shenanigans going on above 
    vec3 dpdx = dFdx(queryPos);
    vec3 dpdy = dFdy(queryPos);

    result.maxDpDx = vec2(dpdx[axes.maximum.y], dpdx[axes.maximum.z]);
    result.maxDpDy = vec2(dpdy[axes.maximum.y], dpdy[axes.maximum.z]);

    result.medDpDx = vec2(dpdx[axes.median.y], dpdx[axes.median.z]);
    result.medDpDy = vec2(dpdy[axes.median.y], dpdy[axes.median.z]);

    // Weights are selected by most relevant axes
    result.mainWeight = weights[axes.maximum.x];
    result.medianWeight = weights[axes.median.x];

    return result;
}

// A simple linear blend with normalization
vec3 ComputeWeights(vec3 normal) {
    // This one has a region where there is no blend, creating more defined interpolations
    const float blendBias = 0.2;
    vec3 blend = abs(normal.xyz);
    blend = max(blend - blendBias, vec3(0.0));
    blend = blend * blend;
    blend /= (blend.x + blend.y + blend.z);
    return blend;
}

vec4 BiplanarTexture(sampler2D tex, float scaler, highp vec3 pos, lowp vec3 normal) {
    // We sort triplanar plane relevance by the relative ordering of the weights and not by the normal
    vec3 weights = ComputeWeights(normal);
    BiplanarAxes axes = ComputeBiplanarPlanes(weights);
    BiplanarData queryData = GenerateBiplanarData(axes, scaler, pos, weights);

    vec4 mainPlaneSample = textureGrad( tex, queryData.maxPos, queryData.maxDpDx, queryData.maxDpDy );
    vec4 secondaryPlaneSample = textureGrad( tex, queryData.medPos, queryData.medDpDx, queryData.medDpDy );
    
    // optional - add local support (prevents discontinuty)
    const float kSupportThreshold = 0.25;
    queryData.mainWeight = clamp( (queryData.mainWeight - kSupportThreshold) / (1.0 - kSupportThreshold), 0.0, 1.0 );
    queryData.medianWeight = clamp( (queryData.medianWeight - kSupportThreshold) / (1.0 - kSupportThreshold), 0.0, 1.0 );

	return (mainPlaneSample * queryData.mainWeight + secondaryPlaneSample * queryData.medianWeight) / (queryData.mainWeight + queryData.medianWeight);
}

vec3 UnpackNormal(vec2 packedNormal, vec2 scale) {
    float x = (packedNormal.x * 2.0 - 1.0) * scale.x;
    float y = (packedNormal.y * 2.0 - 1.0) * scale.y;
    return vec3(x, y, sqrt(clamp(1.0 - x * x - y * y, 0.0, 1.0)));
}

vec2 swizzleIvec(vec3 x, ivec2 i) {
    return vec2( x[i[0]], x[i[1]] );
}

vec3 swizzleIvec(vec3 x, ivec3 i) {
    return vec3( x[i[0]], x[i[1]], x[i[2]] );
}

// This is a whiteout blended tripalanar normal mapping, where each plane's tangent frame is
// approximated by the appropriate sequence of flips of world space axes. For more details
// refer to https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a .
// The only twist here is that we only ever select two planes to do the normal mapping.
vec3 BiplanarNormalMap(sampler2D normalMap, float scaler, highp vec3 pos, lowp vec3 normal, float normalIntensity) {
    // We sort triplanar plane relevance by the relative ordering of the weights and not by the normal
    vec3 weights = ComputeWeights(normal);
    BiplanarAxes axes = ComputeBiplanarPlanes(weights);
    BiplanarData queryData = GenerateBiplanarData(axes, scaler, pos, weights);

    // Tangent space normal maps in a quasi world space. 2-channel XY TS normal texture: this saves 33% on storage (but we have to start using actual 2 channel textures for that)
    int maxAxis = axes.maximum.x;
    int medAxis = axes.median.x;
    vec3 tNormalMax = UnpackNormal(textureGrad(normalMap, queryData.maxPos, queryData.maxDpDx, queryData.maxDpDy).xy, vec2(SignNoZero(normal[maxAxis]) * normalIntensity, normalIntensity) );
    vec3 tNormalMed = UnpackNormal(textureGrad(normalMap, queryData.medPos, queryData.medDpDx, queryData.medDpDy).xy, vec2(SignNoZero(normal[medAxis]) * normalIntensity, normalIntensity) );

    // Swizzle the above normals to tangent space and apply Whiteout blend
    const ivec2 tangentSwizzles[3] = ivec2[3](ivec2(1, 2), ivec2(0, 2), ivec2(1, 0)); // YZ, XZ, YX
    const vec2 tangentMultipliers[3] = vec2[3]( vec2(1, 1), vec2(-1, 1), vec2(1, -1) );
    tNormalMax = vec3(tNormalMax.xy + swizzleIvec(normal, tangentSwizzles[maxAxis]) * tangentMultipliers[maxAxis] * vec2( SignNoZero(normal[maxAxis]),  1), abs(tNormalMax.z) * abs(normal[maxAxis]));
    tNormalMed = vec3(tNormalMed.xy + swizzleIvec(normal, tangentSwizzles[medAxis]) * tangentMultipliers[medAxis] * vec2( SignNoZero(normal[medAxis]),  1), abs(tNormalMed.z) * abs(normal[medAxis]));

    // Swizzle tangent normals to match world orientation
    const ivec3 worldSwizzles[3] = ivec3[3](ivec3(2, 0, 1), ivec3(0, 2, 1), ivec3(1, 0, 2));
    vec3 worldMultipliers[3] = vec3[3]( vec3(SignNoZero(normal.x), SignNoZero(normal.x), 1),
                                          vec3(-SignNoZero(normal.y), SignNoZero(normal.y), 1),
                                          vec3(-1, SignNoZero(normal.z), SignNoZero(normal.z)) );
    // Blend and normalize
    vec3 r = swizzleIvec(tNormalMax, worldSwizzles[maxAxis]) * queryData.mainWeight * worldMultipliers[maxAxis] +
             swizzleIvec(tNormalMed, worldSwizzles[medAxis]) * queryData.medianWeight * worldMultipliers[medAxis];
    return normalize(r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Custom code to evaluate the materials
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ApplyNormalMap(inout MaterialInputs material, inout FragmentData fragmentData) {
#if defined(MATERIAL_HAS_NORMAL)
    if (materialParams.useNormalTexture == 1) {
        // We combine the normals in world space, hence the transformation in the end from world to tangent, assuming an
        // orthonormal tangent frame (which may not hold actually but looks fine enough for now).
        vec3 normalWS = BiplanarNormalMap(materialParams_normalTexture,
                                          materialParams.textureScaler.y,
                                          fragmentData.pos,
                                          fragmentData.normal,
                                          materialParams.normalIntensity);
#if defined(SHAPR_USE_WORLD_NORMALS)
        material.normal = normalWS;
#else
        material.normal = normalWS * getWorldTangentFrame();
#endif // SHAPR_USE_WORLD_NORMALS
    } else {
#if defined(SHAPR_USE_WORLD_NORMALS)
        // We need normals to be in world space
        material.normal = normalize(getWorldTangentFrame() * material.normal);
#endif
    }
#endif
}

void ApplyClearCoatNormalMap(inout MaterialInputs material, inout FragmentData fragmentData) {
#if defined(MATERIAL_HAS_CLEAR_COAT_NORMAL)
    if (materialParams.useClearCoatNormalTexture == 1) {
        vec3 clearCoatNormalWS = BiplanarNormalMap(materialParams_clearCoatNormalTexture,
                                                   materialParams.textureScaler.z,
                                                   fragmentData.pos,
                                                   fragmentData.normal,
                                                   materialParams.clearCoatNormalIntensity);
#if defined(SHAPR_USE_WORLD_NORMALS)
        material.clearCoatNormal = clearCoatNormalWS;
#else
        material.clearCoatNormal = clearCoatNormalWS * getWorldTangentFrame();
#endif // SHAPR_USE_WORLD_NORMALS
    } else {
#if defined(SHAPR_USE_WORLD_NORMALS)
        // We need normals to be in world space
        material.clearCoatNormal = normalize(getWorldTangentFrame() * material.clearCoatNormal);
#endif
    }
#endif
}

void ApplyBaseColor(inout MaterialInputs material, inout FragmentData fragmentData) {
#if defined(MATERIAL_HAS_BASE_COLOR)
    if (materialParams.useBaseColorTexture == 1) {
#if defined(BLENDING_ENABLED) || defined(HAS_REFRACTION)
        material.baseColor.rgba = BiplanarTexture(materialParams_baseColorTexture,
                                                materialParams.textureScaler.x,
                                                fragmentData.pos,
                                                fragmentData.normal)
                                    .rgba;
#else
        material.baseColor.rgb = BiplanarTexture(materialParams_baseColorTexture,
                                                 materialParams.textureScaler.x,
                                                 fragmentData.pos,
                                                 fragmentData.normal)
                                    .rgb;
#endif
    } else {
#if defined(BLENDING_ENABLED) || defined(HAS_REFRACTION)
        material.baseColor.rgba = materialParams.baseColor.rgba;
#else
        material.baseColor.rgb = materialParams.baseColor.rgb;
#endif
    }

    // Naive multiplicative tinting seems to be fine enough for now
    material.baseColor.rgb *= materialParams.tintColor.rgb;
#if defined(BLENDING_ENABLED)
    material.baseColor.rgb *= material.baseColor.a;
    material.baseColor.a = 0.0;
#else
    material.baseColor.a = 1.0;
#endif
#endif
}

void ApplyOcclusion(inout MaterialInputs material, inout FragmentData fragmentData) {
#if defined(MATERIAL_HAS_AMBIENT_OCCLUSION) && defined(BLENDING_DISABLED)
    if (materialParams.useOcclusionTexture == 1) {
        material.ambientOcclusion = BiplanarTexture(materialParams_occlusionTexture,
                                                    materialParams.textureScaler.y,
                                                    fragmentData.pos,
                                                    fragmentData.normal).r;
    } else {
        material.ambientOcclusion = materialParams.occlusion;
    }
    material.ambientOcclusion = clamp(1.0 - materialParams.occlusionIntensity * (1.0 - material.ambientOcclusion), 0.0, 1.0);
#endif
}

void ApplyRoughness(inout MaterialInputs material, inout FragmentData fragmentData) {
#if defined(MATERIAL_HAS_ROUGHNESS)
    if (materialParams.useRoughnessTexture == 1) {
        material.roughness = BiplanarTexture(materialParams_roughnessTexture,
                                             materialParams.textureScaler.y,
                                             fragmentData.pos,
                                             fragmentData.normal).r;
    } else {
        material.roughness = materialParams.roughness;
    }
    material.roughness *= materialParams.roughnessScale;
#endif
}

void ApplyReflectance(inout MaterialInputs material, inout FragmentData fragmentData) {
    // Unfortunately, MATERIAL_HAS_REFLECTANCE is defined even for cloth which does not have reflectance. Luckily, cloth and
    // specular-glossiness models are the only two that do not have this property (and the latter is a legacy mode anyway).
#if defined(MATERIAL_HAS_REFLECTANCE) && !defined(SHADING_MODEL_CLOTH) && !defined(SHADING_MODEL_SPECULAR_GLOSSINESS)
    // Careful: reflectance affects non-metals, so this alone may still make things overly shiny - we added
    // specularIntensity to handle specular response on all lighting paths.
    material.reflectance = clamp(1.0 - material.roughness, 0.0, 1.0);
#endif
}

void ApplyMetallic(inout MaterialInputs material, inout FragmentData fragmentData) {
    // Cloth and specular glossiness explicitly do not have these properties.
#if defined(MATERIAL_HAS_METALLIC) && !defined(SHADING_MODEL_CLOTH) && !defined(SHADING_MODEL_SPECULAR_GLOSSINESS)
    if (materialParams.useMetallicTexture == 1) {
        material.metallic = BiplanarTexture(materialParams_metallicTexture,
                                            materialParams.textureScaler.y,
                                            fragmentData.pos,
                                            fragmentData.normal).r;
    } else {
        material.metallic = materialParams.metallic;
    }
#endif
}

void ApplyClearCoatRoughness(inout MaterialInputs material, inout FragmentData fragmentData) {
#if defined(MATERIAL_HAS_CLEAR_COAT_ROUGHNESS)
    if (materialParams.useClearCoatRoughnessTexture == 1) {
        material.clearCoatRoughness = BiplanarTexture(materialParams_clearCoatRoughnessTexture,
                                                       materialParams.textureScaler.z,
                                                       fragmentData.pos,
                                                       fragmentData.normal).r;
    } else {
        material.clearCoatRoughness = materialParams.clearCoatRoughness;
    }
#endif
}

void ApplyAbsorption(inout MaterialInputs material, inout FragmentData fragmentData) {
    // This is a transmission-only property and those materials actually disable blending
#if defined(MATERIAL_HAS_ABSORPTION) && defined(HAS_REFRACTION)
    material.absorption = ( materialParams.doDeriveAbsorption == 1 ) ? 1.0 - material.baseColor.rgb : materialParams.absorption;
#endif
}

void ApplyIOR(inout MaterialInputs material, inout FragmentData fragmentData) {
    // This is a transmission-only property and those materials actually disable blending
#if defined(MATERIAL_HAS_IOR) && defined(HAS_REFRACTION)
    material.ior = 1.0 + materialParams.iorScale * ( materialParams.ior - 1.0 );
#endif
}

void ApplyTransmission(inout MaterialInputs material, inout FragmentData fragmentData) {
    // This is a transmission-only property and those materials actually disable blending
#if defined(MATERIAL_HAS_TRANSMISSION) && defined(HAS_REFRACTION)
    if ( materialParams.useTransmissionTexture == 1 ) {
        material.transmission = BiplanarTexture(materialParams_transmissionTexture, materialParams.textureScaler.w, fragmentData.pos, fragmentData.normal).r;
    } else {
        material.transmission = materialParams.transmission;
    }
#endif
}

void ApplyThickness(inout MaterialInputs material, inout FragmentData fragmentData) {
    // This is a transmission-only property and those materials actually disable blending
    // This applies both micro and regular thickness, although we only do the latter for now (the former would be used in transparent thin materials).
#if defined(BLENDING_DISABLED) && defined(HAS_REFRACTION)
    float thicknessValue = 0.0;
    if (materialParams.useThicknessTexture == 1) {
        thicknessValue = BiplanarTexture(materialParams_thicknessTexture,
                                          materialParams.textureScaler.w,
                                          fragmentData.pos,
                                          fragmentData.normal).r;
    } else {
        thicknessValue = materialParams.thickness;
    }
    thicknessValue *= materialParams.maxThickness;

#if defined(MATERIAL_HAS_MICRO_THICKNESS) && defined(REFRACTION_TYPE) && REFRACTION_TYPE == REFRACTION_TYPE_THIN
    material.microThickness = thicknessValue; // default 0.0
#elif defined(HAS_REFRACTION)
    material.thickness = thicknessValue; // default 0.5
#endif
#endif
}

void ApplySheenRoughness(inout MaterialInputs material, inout FragmentData fragmentData) {
#if defined(MATERIAL_HAS_SHEEN_ROUGHNESS) && !defined(SHADING_MODEL_SUBSURFACE) && !defined(SHADING_MODEL_CLOTH) &&    \
    defined(BLENDING_DISABLED)
#if defined(MATERIAL_HAS_USE_SHEEN_ROUGHNESS_TEXTURE)
    if (materialParams.useSheenRoughnessTexture == 1) {
        material.sheenRoughness =
            BiplanarTexture(materialParams_sheenRoughnessTexture, 1.0f, fragmentData.pos, fragmentData.normal).r;
    } else {
        material.sheenRoughness = materialParams.sheenRoughness;
    }
#else
    material.sheenRoughness = materialParams.sheenRoughness;
#endif
#endif
}

void ApplyShaprScalars(inout MaterialInputs material, inout FragmentData fragmentData) {
    // All of our materials have specularIntensity and useWard, so no need to define-guard these
    material.specularIntensity = materialParams.specularIntensity;
    material.useWard = (materialParams.useWard == 1) ? true : false;
}

// As all-black cloth materials require positive sheen contribution in order to be visible (read: not pitch black),
// this function should be re-normalizing the final luminance (estimate) of the auto-computed sheen color if it 
// would be too dark otherwise. However, we'd still need to cater for the special case of denormal and all-black
// incoming colors, so I've ultimately decided to just clamp the result of the sqrt.
vec3 BaseColorToSheenColor(vec3 baseColor) {
    //  This epsilon is selected such that visuals on Cloth are cca. matching a perfectly black, roughness = 1 Opaque material
    const float LUMINANCE_THRESHOLD = sqrt(5e-3);
    vec3 result = max( sqrt(baseColor.rgb), vec3(LUMINANCE_THRESHOLD) );
    return result;
}

void ApplyNonTextured(inout MaterialInputs material, inout FragmentData fragmentData) {
#if defined(MATERIAL_HAS_CLEAR_COAT)
    material.clearCoat = materialParams.clearCoat;
#endif
#if defined(MATERIAL_HAS_ANISOTROPY) && defined(BLENDING_DISABLED)
    material.anisotropy = materialParams.anisotropy;
#endif
#if defined(MATERIAL_HAS_ANISOTROPY_DIRECTION) && defined(BLENDING_DISABLED)
    material.anisotropyDirection = materialParams.anisotropyDirection;
#endif
#if defined(MATERIAL_HAS_SHEEN_COLOR) && !defined(SHADING_MODEL_SUBSURFACE) && defined(BLENDING_DISABLED)
    // Subsurface and transparent are not using sheen color but the others are
    material.sheenColor = 
        (materialParams.doDeriveSheenColor == 1) ? BaseColorToSheenColor(material.baseColor.rgb) : materialParams.sheenColor;
    material.sheenColor *= materialParams.sheenIntensity;
#endif
#if defined(MATERIAL_HAS_SUBSURFACE_COLOR) && ( defined(SHADING_MODEL_SUBSURFACE) || defined(SHADING_MODEL_CLOTH) )
    if (materialParams.doDeriveSubsurfaceColor == 1) {
        material.subsurfaceColor = material.baseColor.rgb;
    } else {
        material.subsurfaceColor = materialParams.subsurfaceColor;
    }
    // note: this also incorporates the subsurface intensity, i.e. subsurfaceTint = subsurfaceTintColor * subsurfaceIntensity
    material.subsurfaceColor *= materialParams.subsurfaceTint; 
#endif
#if defined(MATERIAL_HAS_SUBSURFACE_POWER) && defined(SHADING_MODEL_SUBSURFACE)
    material.subsurfacePower = materialParams.subsurfacePower;
#endif
#if defined(MATERIAL_HAS_POST_LIGHTING_COLOR)
    material.postLightingColor = materialParams.ambientColor;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Convenience functions to simplify the various material files. These basically batch together all calls to the
// functions above, relying on the proper define-s to avoid trying to manipulate attributes that are not present.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ApplyAllPrePrepare(inout MaterialInputs material, inout FragmentData fragmentData) {
    // Filament material docs state normals should be set _before_ calling prepareMaterial,
    // and that also means clear coat normals
    ApplyNormalMap(material, fragmentData);
    ApplyClearCoatNormalMap(material, fragmentData);
}

void ApplyAllPostPrepare(inout MaterialInputs material, inout FragmentData fragmentData) {
    ApplyOcclusion(material, fragmentData);
    ApplyBaseColor(material, fragmentData);
    ApplyRoughness(material, fragmentData);
    ApplyReflectance(material, fragmentData);
    ApplyMetallic(material, fragmentData);

    ApplyClearCoatRoughness(material, fragmentData);
    ApplySheenRoughness(material, fragmentData);

    ApplyAbsorption(material, fragmentData);
    ApplyIOR(material, fragmentData);
    ApplyTransmission(material, fragmentData);
    ApplyThickness(material, fragmentData);

    ApplyNonTextured(material, fragmentData);
    ApplyShaprScalars(material, fragmentData);
}