#include <viewer/TweakableMaterial.h>

TweakableMaterial::TweakableMaterial() {
    mSpecularIntensity.value = 1.0f;    
    mAnisotropyDirection.value = { 1.0f, 0.0f, 0.0f };
    mSubsurfaceColor.value = { 0.0f, 0.0f, 0.0f };
    mSubsurfaceTint.value = { 1.0f, 1.0f, 1.0f };
    mMaxThickness.value = 1.0f;
    mIor.value = 1.5f;
    mIorScale.value = 1.0f;
    mNormalIntensity.value = 1.0f;
    mRoughnessScale.value = 1.0f;
    mRoughnessUvScaler.value = 1.0f;
    mOcclusionIntensity.value = 1.0f;
}

json TweakableMaterial::toJson() {
    json result{};

    //result["materialType"] = mMaterialType;
    result["shaderType"] = mShaderType;

    result["useWard"] = mUseWard;

    writeTexturedToJson(result, "baseColor", mBaseColor);
    result["tintColor"] = mTintColor.value;
    result["emissiveIntensity"] = mEmissiveIntensity.value;
    result["emissiveExposureWeight"] = mEmissiveExposureWeight.value;

    result["normalIntensity"] = mNormalIntensity.value;

    writeTexturedToJson(result, "normalTexture", mNormal);

    result["roughnessScale"] = mRoughnessScale.value;
    result["roughnessUvScaler"] = mRoughnessUvScaler.value;
    writeTexturedToJson(result, "roughness", mRoughness);

    writeTexturedToJson(result, "metallic", mMetallic);

    result["occlusionIntensity"] = mOcclusionIntensity.value;
    writeTexturedToJson(result, "occlusion", mOcclusion);

    result["clearCoat"] = mClearCoat.value;
    result["clearCoatNormalIntensity"] = mClearCoatNormalIntensity.value;
    writeTexturedToJson(result, "clearCoatNormal", mClearCoatNormal);
    writeTexturedToJson(result, "clearCoatRoughness", mClearCoatRoughness);

    result["baseTextureScale"] = mBaseTextureScale;
    result["normalTextureScale"] = mNormalTextureScale;
    result["clearCoatTextureScale"] = mClearCoatTextureScale;
    result["refractiveTextureScale"] = mRefractiveTextureScale;

    result["reflectanceScale"] = mSpecularIntensity.value;

    result["anisotropy"] = mAnisotropy.value;
    result["anisotropyDirection"] = mAnisotropyDirection.value;

    result["sheenColor"] = mSheenColor.value;
    result["sheenIntensity"] = mSheenIntensity.value;
    result["subsurfaceColor"] = mSubsurfaceColor.value;
    result["isSubsurfaceColorDerived"] = mSubsurfaceColor.useDerivedQuantity;
    result["subsurfaceTint"] = mSubsurfaceTint.value;
    result["subsurfaceIntensity"] = mSubsurfaceIntensity.value;
    result["isSheenColorDerived"] = mSheenColor.useDerivedQuantity;
    result["subsurfacePower"] = mSubsurfacePower.value;
    writeTexturedToJson(result, "sheenRoughness", mSheenRoughness);

    // Transparent and refractive attributes
    result["absorption"] = mAbsorption.value;
    result["isAbsorptionDerived"] = mAbsorption.useDerivedQuantity;
    result["iorScale"] = mIorScale.value;
    writeTexturedToJson(result, "ior", mIor);
    result["thickness"] = mThickness.value;
    writeTexturedToJson(result, "transmission", mTransmission);
    result["maxThickness"] = mMaxThickness.value;

    result["doRelease"] = mDoRelease;

    return result;
}

void TweakableMaterial::fromJson(const json& source) {
    // handle renaming materialType to shaderType
    if (source.find("shaderType") != source.cend()) {
        mShaderType = source["shaderType"];
    } else {
        mShaderType = source["materialType"];
    }

    bool isAlpha = (mShaderType == TweakableMaterial::MaterialType::Transparent) || (mShaderType == TweakableMaterial::MaterialType::Refractive);

    readValueFromJson(source, "useWard", mUseWard, false);

    readTexturedFromJson(source, "baseColor", mBaseColor, true, isAlpha, isAlpha ? 4 : 3);
    readValueFromJson(source, "tintColor", mTintColor, { 1.0f, 1.0f, 1.0f });
    readValueFromJson(source, "emissiveIntensity", mEmissiveIntensity, 0.0f);
    readValueFromJson(source, "emissiveExposureWeight", mEmissiveExposureWeight, 0.0f);

    readValueFromJson(source, "normalIntensity", mNormalIntensity, 1.0f);
    readTexturedFromJson(source, "normalTexture", mNormal, false, false, 3);

    readValueFromJson(source, "roughnessScale", mRoughnessScale, 1.0f);
    readValueFromJson(source, "roughnessUvScaler", mRoughnessUvScaler, 1.0f);
    readTexturedFromJson(source, "roughness", mRoughness);

    readTexturedFromJson(source, "metallic", mMetallic);

    readValueFromJson(source, "occlusionIntensity", mOcclusionIntensity.value, 1.0f);
    readTexturedFromJson(source, "occlusion", mOcclusion, 1.0f);

    readValueFromJson(source, "clearCoat", mClearCoat, 0.0f);
    readValueFromJson(source, "clearCoatNormalIntensity", mClearCoatNormalIntensity, 1.0f);
    readTexturedFromJson(source, "clearCoatNormal", mClearCoatNormal, false, false, 3);
    readTexturedFromJson(source, "clearCoatRoughness", mClearCoatRoughness);

    readValueFromJson(source, "baseTextureScale", mBaseTextureScale, 1.0f);
    readValueFromJson(source, "normalTextureScale", mNormalTextureScale, 1.0f);
    readValueFromJson(source, "clearCoatTextureScale", mClearCoatTextureScale, 1.0f);
    readValueFromJson(source, "refractiveTextureScale", mRefractiveTextureScale, 1.0f);
    readValueFromJson(source, "reflectanceScale", mSpecularIntensity, 1.0f);

    readValueFromJson(source, "anisotropy", mAnisotropy, 0.0f);
    readValueFromJson(source, "anisotropyDirection", mAnisotropyDirection, { 1.0f, 0.0f, 0.0f });

    readTexturedFromJson(source, "sheenRoughness", mSheenRoughness);
    readValueFromJson<filament::math::float3, true, true>(source, "sheenColor", mSheenColor, { 0.0f, 0.0f, 0.0f });

    readValueFromJson(source, "isSheenColorDerived", mSheenColor.useDerivedQuantity, false);
    readValueFromJson(source, "sheenIntensity", mSheenIntensity, 1.0f);
    readValueFromJson<filament::math::float3, true>(source, "subsurfaceColor", mSubsurfaceColor, { 0.0f, 0.0f, 0.0f });
    readValueFromJson(source, "isSubsurfaceColorDerived", mSubsurfaceColor.useDerivedQuantity, false);
    readValueFromJson<filament::math::float3, true>(source, "subsurfaceTint", mSubsurfaceTint, { 1.0f, 1.0f, 1.0f });
    readValueFromJson(source, "subsurfaceIntensity", mSubsurfaceIntensity.value, 1.0f);
    readValueFromJson(source, "subsurfacePower", mSubsurfacePower, 1.0f);

    readValueFromJson<filament::math::float3, true, true>(source, "absorption", mAbsorption, { 0.0f, 0.0f, 0.0f });
    readValueFromJson(source, "isAbsorptionDerived", mAbsorption.useDerivedQuantity, false);
    readValueFromJson(source, "iorScale", mIorScale, 1.0f);
    readValueFromJson(source, "ior", mIor);
    readValueFromJson(source, "thickness", mThickness);
    readTexturedFromJson(source, "transmission", mTransmission);
    readValueFromJson(source, "maxThickness", mMaxThickness, 1.0f);

    readValueFromJson(source, "doRelease", mDoRelease, false);

    auto checkAndFixPathRelative([](auto& propertyWithPath) {
        if (propertyWithPath.isFile) {
            utils::Path asPath(propertyWithPath.filename);
            if (asPath.isAbsolute()) {
                std::string newFilePath = asPath.makeRelativeTo(g_ArtRootPathStr).c_str();
                propertyWithPath.filename = newFilePath;
            }
        }
    });

    checkAndFixPathRelative(mBaseColor);
    checkAndFixPathRelative(mNormal);
    checkAndFixPathRelative(mRoughness);
    checkAndFixPathRelative(mMetallic);
    checkAndFixPathRelative(mClearCoatNormal);
    checkAndFixPathRelative(mClearCoatRoughness);
    checkAndFixPathRelative(mSheenRoughness);
    checkAndFixPathRelative(mTransmission);
    checkAndFixPathRelative(mThickness);
    checkAndFixPathRelative(mIor);
}

template <typename T, bool MayContainFile = false, bool IsColor = true, bool IsDerivable = false, typename = IsValidTweakableType<T> >
void resetMemberToValue(TweakableProperty<T, MayContainFile, IsColor, IsDerivable>& prop, T value) {
    prop.value = value;
    prop.isFile = false;
}

void TweakableMaterial::resetWithType(MaterialType newType) {

    resetMemberToValue(mBaseColor, {0.0f, 0.0f, 0.0f, 1.0f});
    resetMemberToValue(mNormal, {});
    resetMemberToValue(mOcclusion, 1.0f);
    resetMemberToValue(mRoughnessScale, 1.0f);
    resetMemberToValue(mRoughnessUvScaler, 1.0f);
    resetMemberToValue(mRoughness, 0.0f);
    resetMemberToValue(mMetallic, {});

    resetMemberToValue(mClearCoat, {});
    resetMemberToValue(mClearCoatNormal, {});
    resetMemberToValue(mClearCoatRoughness, {});

    mRequestedTextures = {};

    resetMemberToValue(mTintColor, { 1.0f, 1.0f, 1.0f });
    resetMemberToValue(mEmissiveExposureWeight, 0.0f);
    resetMemberToValue(mEmissiveIntensity, 0.0f);

    mBaseTextureScale = 1.0f;
    mNormalTextureScale = 1.0f;
    mClearCoatTextureScale = 1.0f;
    mRefractiveTextureScale = 1.0f;
    resetMemberToValue(mSpecularIntensity, 1.0f);
    resetMemberToValue(mNormalIntensity, 1.0f);

    resetMemberToValue(mAnisotropy, {});
    resetMemberToValue(mAnisotropyDirection, { 1.0f, 0.0f, 0.0f });

    resetMemberToValue(mSubsurfaceColor, { 0.0f, 0.0f, 0.0f });
    resetMemberToValue(mSubsurfaceTint, { 1.0f, 1.0f, 1.0f });
    resetMemberToValue(mSubsurfaceIntensity, 1.0f);
    resetMemberToValue(mSheenColor, {});
    resetMemberToValue(mSheenIntensity, {});
    resetMemberToValue(mSheenRoughness, {});

    resetMemberToValue(mSubsurfacePower, 1.0f);

    resetMemberToValue(mAbsorption, {});
    resetMemberToValue(mTransmission, {});
    resetMemberToValue(mMaxThickness, 1.0f);
    resetMemberToValue(mThickness, {});
    resetMemberToValue(mIorScale, 1.0f);
    resetMemberToValue(mIor, 1.5f);

    mAbsorption.useDerivedQuantity = false;
    mSheenColor.useDerivedQuantity = false;
    mUseWard = false;
    mDoRelease = false;

    mShaderType = newType;
}

void TweakableMaterial::drawUI(const std::string& header) {
    if (ImGui::CollapsingHeader("Integration")) {
        if (ImGui::Checkbox("Release material", &mDoRelease)) {
            // The button was pressed, so request validation if it turned from false to true
            if (mDoRelease) {
                mDoesRequireValidation = true;
            }
        }
        if (header.size() > 0) {
            ImGui::TextColored({ 1,0,0,1 }, "%s", header.c_str());
        }
        else {
            ImGui::TextColored({ 0,1,0,1 }, "OK");
        }
    }
    if (ImGui::CollapsingHeader("Base color")) {
        ImGui::SliderFloat("Tile: albedo texture", &mBaseTextureScale, 1.0f / 1024.0f, 32.0f);
        ImGui::Separator();

        mBaseColor.addWidget("baseColor");
        if (mBaseColor.isFile) {
            bool isAlpha = (mShaderType == MaterialType::Transparent || mShaderType == MaterialType::Refractive);
            enqueueTextureRequest(mBaseColor, true, isAlpha, isAlpha ? 4 : 3);
        }

        mTintColor.addWidget("tintColor");

        mEmissiveIntensity.addWidget("emissive intensity", 0.0f, 1000.0f);
        mEmissiveExposureWeight.addWidget("exposure weight on emissive");
    }

    if (ImGui::CollapsingHeader("Normal, roughness, specular, metallic")) {
        ImGui::SliderFloat("Tile: normal et al. textures", &mNormalTextureScale, 1.0f / 1024.0f, 32.0f);
        ImGui::Separator();

        mNormalIntensity.addWidget("normal intensity", 0.0f, 2.0f);
        
        mNormal.addWidget("normal");
        if (mNormal.isFile) enqueueTextureRequest(mNormal, false, false, 3);

        mRoughnessScale.addWidget("roughness scale", 0.0f, 3.0f);
        mRoughnessUvScaler.addWidget("roughness UV multiplier", 0.125f, 4.0f);

        mRoughness.addWidget("roughness");
        if (mRoughness.isFile) enqueueTextureRequest(mRoughness);

        mSpecularIntensity.addWidget("specular intensity", 0.0f, 3.0f);

        if (mShaderType != MaterialType::Cloth) {
            mMetallic.addWidget("metallic");
            if (mMetallic.isFile) enqueueTextureRequest(mMetallic);
        }

        mOcclusionIntensity.addWidget("occlusion intensity");
        mOcclusion.addWidget("occlusion");
        if (mOcclusion.isFile) enqueueTextureRequest(mOcclusion);
    }

    if (ImGui::CollapsingHeader("Clear coat settings")) {
        ImGui::SliderFloat("Tile: clearCoat et al. textures", &mClearCoatTextureScale, 1.0f / 1024.0f, 32.0f);
        ImGui::Separator();

        mClearCoat.addWidget("clearCoat intensity");

        mClearCoatNormalIntensity.addWidget("clearCoat normal intensity");
        mClearCoatNormal.addWidget("clearCoat normal");
        if (mClearCoatNormal.isFile) enqueueTextureRequest(mClearCoatNormal, false, false, 3);

        mClearCoatRoughness.addWidget("clearCoat roughness");
        if (mClearCoatRoughness.isFile) enqueueTextureRequest(mClearCoatRoughness);
    }

    switch (mShaderType) {
    case MaterialType::Opaque: {
        if (ImGui::CollapsingHeader("Sheen settings")) {
            mSheenColor.addWidget("sheen color");
            if (mSheenColor.useDerivedQuantity) {
                mSheenIntensity.addWidget("auto-sheen intensity");
            }
            mSheenRoughness.addWidget("sheen roughness");
            if (mSheenRoughness.isFile) enqueueTextureRequest(mSheenRoughness);
        }

        if (ImGui::CollapsingHeader("Metal (anisotropy, etc.) settings")) {
            mAnisotropy.addWidget("anisotropy", -1.0f, 1.0f);
            
            // This is more intuitive to toggle like this
            ImGui::Separator();
            ImGui::LabelText("anisotropy direction", "anisotropy direction");
            ImGuiExt::DirectionWidget("anisotropyDirection", mAnisotropyDirection.value.v);
        }
        break;
    }
    // For backward compatibility and warning supression (the enum value needs to be kept)
    case MaterialType::Transparent: {
        break;
    }
    case MaterialType::Refractive: {
        if (ImGui::CollapsingHeader("Refractive properties")) {
            ImGui::SliderFloat("Tile: refractive textures", &mRefractiveTextureScale, 1.0f / 1024.0f, 32.0f);
            ImGui::Separator();

            mIorScale.addWidget("ior scale", 0.0f, 4.0f);
            mIor.addWidget("ior", 1.0f, 2.0f);
            mAbsorption.addWidget("absorption");
            mTransmission.addWidget("transmission");
            mMaxThickness.addWidget("thickness scale", 1.0f, 32.0f);
            mThickness.addWidget("thickness");
        }
        if (ImGui::CollapsingHeader("Metal (anisotropy, etc.) settings")) {
            mAnisotropy.addWidget("anisotropy", -1.0f, 1.0f);

            // This is more intuitive to toggle like this
            ImGui::Separator();
            ImGui::LabelText("anisotropy direction", "anisotropy direction");
            ImGuiExt::DirectionWidget("anisotropyDirection", mAnisotropyDirection.value.v);
        }
        break;
    }
    case MaterialType::Cloth: {
        if (ImGui::CollapsingHeader("Cloth settings")) {
            mSubsurfaceColor.addWidget("subsurface color");
            mSubsurfaceTint.addWidget("subsurface tint");
            mSubsurfaceIntensity.addWidget("subsurface intensity");

            mSheenColor.addWidget("sheen color");
            if (mSheenColor.useDerivedQuantity) {
                mSheenIntensity.addWidget("auto-sheen intensity");
            }
        }
        break;
    }
    case MaterialType::Subsurface: {
        if (ImGui::CollapsingHeader("Subsurface settings")) {
            mSubsurfaceColor.addWidget("subsurface color");
            mSubsurfaceTint.addWidget("subsurface tint");
            mSubsurfaceIntensity.addWidget("subsurface intensity");

            mSubsurfacePower.addWidget("subsurface power", 0.125f, 16.0f);

            mMaxThickness.addWidget("thickness scale", 1.0f, 32.0f);
            mThickness.addWidget("thickness");
        }
        break;
    }
    }

    if (ImGui::CollapsingHeader("Shader setup")) {
        ImGui::Checkbox("Use Ward specular normal distribution", &mUseWard);
    }
}

const TweakableMaterial::RequestedTexture TweakableMaterial::nextRequestedTexture() {
    // TODO: this is very wasteful, make a constant size vector allocation and use an index to track where we are ASAP
    while (mRequestedTextures.size() > 0 && mRequestedTextures.back().filename == "") {
        mRequestedTextures.pop_back();
    }

    RequestedTexture lastRequest{};
    if (mRequestedTextures.size() > 0) {
        lastRequest = mRequestedTextures.back();
        mRequestedTextures.pop_back();
    } 

    return lastRequest;
}

void TweakableMaterial::enqueueTextureRequest(const std::string& filename, bool doRequestReload, bool isSrgb, bool isAlpha, int channelCount) {
    mRequestedTextures.push_back({ filename, channelCount, isSrgb, isAlpha, doRequestReload });
}
