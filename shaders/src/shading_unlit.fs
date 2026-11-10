void addEmissive(const MaterialInputs material, inout vec4 color) {
#if defined(MATERIAL_HAS_EMISSIVE)
    highp vec4 emissive = material.emissive;
    highp float attenuation = mix(1.0, getExposure(), emissive.w);
#if defined(BLEND_MODE_TRANSPARENT) || defined(BLEND_MODE_FADE)
    attenuation *= color.a;
#endif
    color.rgb += emissive.rgb * attenuation;
#endif
}

vec4 fixupAlpha(vec4 color) {
#if defined(BLEND_MODE_MASKED)
    // If we reach this point in the code, we already know that the fragment is not discarded due
    // to the threshold factor. Therefore we can just output 1.0, which prevents a "punch through"
    // effect from occuring. We do this only for TRANSLUCENT views in order to prevent breakage
    // of ALPHA_TO_COVERAGE.
    return vec4(color.rgb, (frameUniforms.needsAlphaChannel == 1.0) ? 1.0 : color.a);
#else
    return color;
#endif
}

#if defined(MATERIAL_HAS_SHADOW_MULTIPLIER) && defined(VARIANT_HAS_SHADOWING) && defined(VARIANT_HAS_DYNAMIC_LIGHTING)
// Shapr3D: shadows of point and spot lights on shadow-only surfaces. These are copies of the
// froxel and attenuation helpers of light_punctual.fs, which unlit materials don't include.

uint shadowMultiplier_getFroxelIndex(const highp vec3 fragCoords) {
    uvec3 froxelCoord;
    froxelCoord.xy = uvec2(fragCoords.xy * frameUniforms.froxelCountXY);
    highp float viewSpaceNormalizedZ = frameUniforms.zParams.x * fragCoords.z + frameUniforms.zParams.y;
    float zSliceCount = frameUniforms.zParams.w;
    float sliceZWithoutOffset = log2(viewSpaceNormalizedZ) * frameUniforms.zParams.z;
    froxelCoord.z = uint(clamp(sliceZWithoutOffset + zSliceCount, 0.0, zSliceCount - 1.0));
    return froxelCoord.x * frameUniforms.fParams.x +
           froxelCoord.y * frameUniforms.fParams.y +
           froxelCoord.z * frameUniforms.fParams.z;
}

uint shadowMultiplier_getLightIndex(const uint index) {
    uint v = index >> 4u;
    uint c = (index >> 2u) & 0x3u;
    uint s = (index & 0x3u) * 8u;
    highp uvec4 d = froxelRecordUniforms.records[v];
    return (d[c] >> s) & 0xFFu;
}

float shadowMultiplier_getDistanceAttenuation(const highp vec3 posToLight, float falloff) {
    float distanceSquare = dot(posToLight, posToLight);
    float factor = distanceSquare * falloff;
    float smoothFactor = saturate(1.0 - factor * factor);
    float attenuation = smoothFactor * smoothFactor;
    highp vec3 v = getWorldPosition() - getWorldCameraPosition();
    attenuation *= saturate(frameUniforms.lightFarAttenuationParams.x - dot(v, v) * frameUniforms.lightFarAttenuationParams.y);
    return attenuation / max(distanceSquare, 1e-4);
}

/**
 * Adds to illuminance the luminance of the light that the shadow-casting point and spot lights would deliver to
 * the fragment without shadows, and to shadowedIlluminance the part of it their shadows block.
 */
void accumulatePunctualLightShadows(const highp vec3 n,
        inout float illuminance, inout float shadowedIlluminance) {
    highp uint f;
    {
        uint froxelIndex = shadowMultiplier_getFroxelIndex(getNormalizedPhysicalViewportCoord());
        highp uvec4 d = froxelsUniforms.records[froxelIndex >> 2u];
        f = d[froxelIndex & 0x3u];
    }
    uint index = f >> 16u;
    uint end = index + (f & 0xFFu);
    int channels = object_uniforms_flagsChannels & 0xFF;
    highp vec3 worldPosition = getWorldPosition();

    for ( ; index < end; index++) {
        highp mat4 data = lightsUniforms.lights[shadowMultiplier_getLightIndex(index)];
        highp uint typeShadow = floatBitsToUint(data[3][2]);
        highp uint lightChannels = floatBitsToUint(data[3][3]);
        // A light that casts no shadows would only weaken the shadows of the other lights, since a
        // shadow-only surface can't show the light it adds
        bool castsShadows = bool(lightChannels & 0x10000u);
        if ((int(lightChannels) & channels) == 0 || !castsShadows) {
            continue;
        }

        highp vec3 posToLight = data[0].xyz - worldPosition;
        highp vec3 l = normalize(posToLight);
        float NoL = saturate(dot(n, l));
        float attenuation = shadowMultiplier_getDistanceAttenuation(posToLight, data[0].w);
        bool isSpot = (typeShadow & 0x1u) != 0u;
        if (isSpot) {
            float cd = dot(-data[1].xyz, l);
            float angle = saturate(cd * data[2].z + data[2].w);
            attenuation *= angle * angle;
        }
        vec3 color = vec3(unpackHalf2x16(floatBitsToUint(data[2][0])),
                unpackHalf2x16(floatBitsToUint(data[2][1])).x);
        float lightIlluminance = luminance(color) *
                computePreExposedIntensity(data[3][1], frameUniforms.exposure) * attenuation * NoL;
        if (lightIlluminance <= 0.0) {
            continue;
        }

        int shadowIndex = int((typeShadow >> 8u) & 0xFFu);
        if (!isSpot) {
            shadowIndex += getPointLightFace(worldPosition - data[0].xyz);
        }
        highp float zLight = dot(shadowUniforms.shadows[shadowIndex].lightFromWorldZ,
                vec4(worldPosition, 1.0));
        highp vec4 shadowPosition = getShadowPosition(shadowIndex, data[1].xyz, zLight);
        float visibility = shadow(false, light_shadowMap, shadowIndex, shadowPosition, zLight);

        illuminance += lightIlluminance;
        shadowedIlluminance += lightIlluminance * (1.0 - visibility);
    }
}
#endif

#if defined(VARIANT_HAS_DIRECTIONAL_LIGHTING) && defined(VARIANT_HAS_SHADOWING)
float getSunVisibility() {
    float visibility = 1.0;
    int cascade = getShadowCascade();
    bool cascadeHasVisibleShadows = bool(frameUniforms.cascades & ((1 << cascade) << 8));
    bool hasDirectionalShadows = bool(frameUniforms.directionalShadows & 1);
    if (hasDirectionalShadows && cascadeHasVisibleShadows) {
        highp vec4 shadowPosition = getShadowPosition(cascade);
        visibility = shadow(true, light_shadowMap, cascade, shadowPosition, 0.0);
        // shadow far attenuation
        highp vec3 v = getWorldPosition() - getWorldCameraPosition();
        // (viewFromWorld * v).z == dot(transpose(viewFromWorld), v)
        highp float z = dot(transpose(getViewFromWorldMatrix())[2].xyz, v);
        highp vec2 p = frameUniforms.shadowFarAttenuationParams;
        visibility = 1.0 - ((1.0 - visibility) * saturate(p.x - z * z * p.y));
    }
    if ((frameUniforms.directionalShadows & 0x2) != 0 && visibility > 0.0) {
        if ((object_uniforms_flagsChannels & FILAMENT_OBJECT_CONTACT_SHADOWS_BIT) != 0) {
            visibility *= (1.0 - screenSpaceContactShadow(frameUniforms.lightDirection));
        }
    }
    return visibility;
}
#endif

/**
 * Evaluates unlit materials. In this lighting model, only the base color and
 * emissive properties are taken into account:
 *
 * finalColor = baseColor + emissive
 *
 * The emissive factor is only applied if the fragment passes the optional
 * alpha test.
 *
 * When the shadowMultiplier property is enabled on the material, the final
 * color is multiplied by the inverse light visibility to apply a shadow.
 * This is mostly useful in AR to cast shadows on unlit transparent shadow
 * receiving planes.
 */
vec4 evaluateMaterial(const MaterialInputs material) {
    vec4 color = material.baseColor;

#if defined(MATERIAL_HAS_SHADOW_MULTIPLIER) && defined(VARIANT_HAS_SHADOWING) && defined(VARIANT_HAS_DYNAMIC_LIGHTING)
    // Shapr3D: when point or spot lights reach the fragment, the shadow is the part of all the
    // light that is blocked, so each light darkens it as much as it would brighten it.
    // The geometric normal is only set for meshes with tangents, the derivatives give the normal
    // of the flat surfaces shadows are cast on.
    highp vec3 worldPosition = getWorldPosition();
    highp vec3 n = normalize(cross(dFdx(worldPosition), dFdy(worldPosition)));
    n = dot(n, getWorldCameraPosition() - worldPosition) < 0.0 ? -n : n;
    float illuminance = 0.0;
    float shadowedIlluminance = 0.0;
    accumulatePunctualLightShadows(n, illuminance, shadowedIlluminance);
    if (illuminance > 0.0) {
#if defined(VARIANT_HAS_DIRECTIONAL_LIGHTING)
        float sunIlluminance = luminance(frameUniforms.lightColorIntensity.rgb) *
                frameUniforms.lightColorIntensity.w * saturate(dot(n, frameUniforms.lightDirection));
        illuminance += sunIlluminance;
        shadowedIlluminance += sunIlluminance * (1.0 - getSunVisibility());
#endif
        color *= shadowedIlluminance / illuminance;
        addEmissive(material, color);
        return fixupAlpha(color);
    }
#endif

#if defined(VARIANT_HAS_DIRECTIONAL_LIGHTING)
#if defined(VARIANT_HAS_SHADOWING)
    color *= 1.0 - getSunVisibility();
#else
    color = vec4(0.0);
#endif
#elif defined(MATERIAL_HAS_SHADOW_MULTIPLIER)
    color = vec4(0.0);
#endif

    addEmissive(material, color);

    return fixupAlpha(color);
}
