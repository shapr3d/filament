#include <viewer/TweakableMaterial.h>

TweakableMaterial::TweakableMaterial() {
    mSpecularIntensity.value = 1.0f;    
    mAnisotropyDirection.value = { 1.0f, 0.0f, 0.0f };
    mMaxThickness.value = 1.0f;
    mIor.value = 1.5f;
    mIorScale.value = 1.0f;
    mNormalIntensity.value = 1.0f;
    mRoughnessScale.value = 1.0f;
}

json TweakableMaterial::toJson() {
    json result{};

    result["materialType"] = mMaterialType;

    writeTexturedToJson(result, "baseColor", mBaseColor);

    result["normalIntensity"] = mNormalIntensity.value;

    writeTexturedToJson(result, "normalTexture", mNormal);

    result["roughnessScale"] = mRoughnessScale.value;
    writeTexturedToJson(result, "roughness", mRoughness);

    writeTexturedToJson(result, "metallic", mMetallic);

    writeTexturedToJson(result, "occlusion", mOcclusion);

    result["clearCoat"] = mClearCoat.value;
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
    writeTexturedToJson(result, "sheenRoughness", mSheenRoughness);

    // Transparent and refractive attributes
    result["absorption"] = mAbsorption.value;
    result["iorScale"] = mIorScale.value;
    writeTexturedToJson(result, "ior", mIor);
    writeTexturedToJson(result, "thickness", mThickness);
    writeTexturedToJson(result, "transmission", mTransmission);
    result["maxThickness"] = mMaxThickness.value;

    return result;
}

void TweakableMaterial::fromJson(const json& source) {
    try {
        mMaterialType = source["materialType"];

        readTexturedFromJson(source, "baseColor", mBaseColor);

        readValueFromJson(source, "normalIntensity", mNormalIntensity, 1.0f);
        readTexturedFromJson(source, "normalTexture", mNormal);

        readValueFromJson(source, "roughnessScale", mRoughnessScale, 1.0f);
        readTexturedFromJson(source, "roughness", mRoughness);

        readTexturedFromJson(source, "metallic", mMetallic);

        readTexturedFromJson(source, "occlusion", mOcclusion);

        readValueFromJson(source, "clearCoat", mClearCoat, 0.0f);
        readTexturedFromJson(source, "clearCoatNormal", mClearCoatNormal);
        readTexturedFromJson(source, "clearCoatRoughness", mClearCoatRoughness);

        readValueFromJson(source, "baseTextureScale", mBaseTextureScale, 1.0f);
        readValueFromJson(source, "normalTextureScale", mNormalTextureScale, 1.0f);
        readValueFromJson(source, "clearCoatTextureScale", mClearCoatTextureScale, 1.0f);
        readValueFromJson(source, "refractiveTextureScale", mRefractiveTextureScale, 1.0f);
        readValueFromJson(source, "reflectanceScale", mSpecularIntensity, 1.0f);

        readValueFromJson(source, "anisotropy", mAnisotropy, 0.0f);
        readValueFromJson(source, "anisotropyDirection", mAnisotropyDirection, { 1.0f, 0.0f, 0.0f });

        readTexturedFromJson(source, "sheenRoughness", mSheenRoughness);
        readValueFromJson<filament::math::float3, true>(source, "sheenColor", mSheenColor, { 0.0f, 0.0f, 0.0f });

        readValueFromJson<filament::math::float3, true>(source, "absorption", mAbsorption, { 0.0f, 0.0f, 0.0f });
        readValueFromJson(source, "iorScale", mIorScale, 1.0f);
        readTexturedFromJson(source, "ior", mIor);
        readTexturedFromJson(source, "thickness", mThickness);
        readTexturedFromJson(source, "transmission", mTransmission);
        readValueFromJson(source, "maxThickness", mMaxThickness, 1.0f);
    }
    catch (...) {
        std::cout << "Could not load material file.\n";
    }
}

void TweakableMaterial::drawUI() {
    if (ImGui::CollapsingHeader("Base color")) {
        ImGui::SliderFloat("Tile: albedo texture", &mBaseTextureScale, 1.0f / 1024.0f, 32.0f);
        ImGui::Separator();

        mBaseColor.addWidget("baseColor");
        if (mBaseColor.isFile) {
            bool isAlpha = (mMaterialType != MaterialType::Opaque);
            enqueueTextureRequest(mBaseColor, true, isAlpha);
        }
    }

    if (ImGui::CollapsingHeader("Normal, roughness, specular, metallic")) {
        ImGui::SliderFloat("Tile: normal et al. textures", &mNormalTextureScale, 1.0f / 1024.0f, 32.0f);
        ImGui::Separator();

        mNormalIntensity.addWidget("normal intensity", 0.0f, 32.0f);
        
        mNormal.addWidget("normal");
        if (mNormal.isFile) enqueueTextureRequest(mNormal);

        mRoughnessScale.addWidget("roughness scale", 0.0f, 3.0f);

        mRoughness.addWidget("roughness");
        if (mRoughness.isFile) enqueueTextureRequest(mRoughness);

        mSpecularIntensity.addWidget("specular intensity", 0.0f, 3.0f);

        mMetallic.addWidget("metallic");
        if (mMetallic.isFile) enqueueTextureRequest(mMetallic);

        mOcclusion.addWidget("occlusion");
        if (mOcclusion.isFile) enqueueTextureRequest(mOcclusion);
    }

    if (ImGui::CollapsingHeader("Clear coat settings")) {
        ImGui::SliderFloat("Tile: clearCoat et al. textures", &mClearCoatTextureScale, 1.0f / 1024.0f, 32.0f);
        ImGui::Separator();

        mClearCoat.addWidget("clearCoat intensity");

        mClearCoatNormal.addWidget("clearCoat normal");
        if (mClearCoatNormal.isFile) enqueueTextureRequest(mClearCoatNormal);

        mClearCoatRoughness.addWidget("clearCoat roughness");
        if (mClearCoatRoughness.isFile) enqueueTextureRequest(mClearCoatRoughness);
    }

    if (ImGui::CollapsingHeader("Cloth (sheen, etc.) settings")) {
        mSheenColor.addWidget("sheen color");

        mSheenRoughness.addWidget("sheen roughness");
        if (mSheenRoughness.isFile) enqueueTextureRequest(mSheenRoughness);
    }

    if (mMaterialType == MaterialType::Opaque) {
        if (ImGui::CollapsingHeader("Metal (anisotropy, etc.) settings")) {
            mAnisotropy.addWidget("anisotropy", -1.0f, 1.0f);
            mAnisotropyDirection.addWidget("anisotropy direction", -1.0f, 1.0f);
        }
    } else { 
        if (ImGui::CollapsingHeader("Transparent and refractive properties")) {
            ImGui::SliderFloat("Tile: refractive textures", &mRefractiveTextureScale, 1.0f / 1024.0f, 32.0f);
            ImGui::Separator();

            mIorScale.addWidget("ior scale", 0.0f, 4.0f);
            mIor.addWidget("ior", 1.0f, 2.0f);
            mAbsorption.addWidget("absorption");
            mTransmission.addWidget("transmission");
            mMaxThickness.addWidget("thickness scale", 1.0f, 32.0f);
            mThickness.addWidget("thickness");
        }
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

void TweakableMaterial::enqueueTextureRequest(const std::string& filename, bool doRequestReload, bool isSrgb, bool isAlpha) {
    mRequestedTextures.push_back({ filename, isSrgb, isAlpha, doRequestReload });
}
