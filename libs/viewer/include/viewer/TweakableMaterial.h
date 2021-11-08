#pragma once

#include <viewer/TweakableProperty.h>

#include <math/mat4.h>
#include <math/vec3.h>

#include <viewer/json.hpp>
#include <vector>
#include <string>

// The following template specializations have to be defined under the nlohmann namespace
namespace nlohmann {
    template<>
    struct adl_serializer<filament::math::float4> {
        static void to_json(json& j, const filament::math::float4& p) {
            j = json{ {"x", p.x}, {"y", p.y}, {"z", p.z}, {"w", p.w} };
        }

        static void from_json(const json& j, filament::math::float4& p) {
            j.at("x").get_to(p.x);
            j.at("y").get_to(p.y);
            j.at("z").get_to(p.z);
            j.at("w").get_to(p.w);
        }
    };

    template<>
    struct adl_serializer<filament::math::float3> {
        static void to_json(json& j, const filament::math::float3& p) {
            j = json{ {"x", p.x}, {"y", p.y}, {"z", p.z} };
        }

        static void from_json(const json& j, filament::math::float3& p) {
            j.at("x").get_to(p.x);
            j.at("y").get_to(p.y);
            j.at("z").get_to(p.z);
        }
    };

    template<>
    struct adl_serializer<filament::math::float2> {
        static void to_json(json& j, const filament::math::float2& p) {
            j = json{ {"x", p.x}, {"y", p.y} };
        }

        static void from_json(const json& j, filament::math::float2& p) {
            j.at("x").get_to(p.x);
            j.at("y").get_to(p.y);
        }
    };
}

using nlohmann::json;

struct TweakableMaterial {
public:
    TweakableMaterial();

    json toJson();
    void fromJson(const json& source);
    void drawUI();

    const std::string nextRequestedTexture();

    TweakablePropertyTextured<filament::math::float4> mBaseColor{ {0.0f, 0.0f, 0.0f, 1.0f} };

    TweakablePropertyTextured<float, false> mNormal{};
    TweakablePropertyTextured<float> mRoughness{};
    TweakablePropertyTextured<float> mMetallic{};

    TweakableProperty<float> mClearCoat{};
    TweakablePropertyTextured<float> mClearCoatNormal{};
    TweakablePropertyTextured<float> mClearCoatRoughness{};

    std::vector<std::string> mRequestedTextures{};

    float mBaseTextureScale = 1.0f; // applied to baseColor texture
    float mNormalTextureScale = 1.0f; // applied to normal.xy, roughness, and metallic maps
    float mClearCoatTextureScale = 1.0f; // applied to clearcloat normal.xy, roughness, and value textures
    float mRefractiveTextureScale = 1.0f; // applied to ior, transmission, {micro}Thickness textures
    TweakableProperty<float> mReflectanceScale{}; // scales Filament's reflectance attribute - should be equivalent to specular in Blender

    TweakableProperty<float> mAnisotropy{}; // for metals
    TweakableProperty<filament::math::float3, false, false> mAnisotropyDirection{}; // for metals; not color

    TweakableProperty<filament::math::float3> mSheenColor{}; // for cloth
    TweakablePropertyTextured<float> mSheenRoughness{}; // for cloth

    TweakableProperty<filament::math::float3> mAbsorption{}; // for refractive
    TweakablePropertyTextured<float> mTransmission{}; // for refractive
    TweakableProperty<float> mMaxThickness{}; // for refractive; this scales the values read from a thickness property/texture
    TweakablePropertyTextured<float> mThickness{}; // for refractive
    TweakableProperty<float> mIorScale{}; // for refractive
    TweakablePropertyTextured<float> mIor{}; // for refractive

    enum MaterialType { Opaque, TransparentSolid, TransparentThin};
    MaterialType mMaterialType{};

private:
    void enqueueTextureRequest(const std::string& filename);

    template< typename T, bool MayContainFile = false, bool IsColor = true, typename = IsValidTweakableType<T> >
    void writeTexturedToJson(json& result, const std::string& prefix, const TweakableProperty<T, MayContainFile, IsColor>& item) {
        result[prefix] = item.value;
        result[prefix + "IsFile"] = item.isFile;
        result[prefix + "Texture"] = item.filename;
    }

    template< typename T, bool MayContainFile = false, bool IsColor = true, typename = IsValidTweakableType<T> >
    void readTexturedFromJson(const json& source, const std::string& prefix, TweakableProperty<T, MayContainFile, IsColor>& item) {
        item.value = source[prefix];
        item.isFile = source[prefix + "IsFile"];
        item.filename = source[prefix + "Texture"];
        if (item.isFile) enqueueTextureRequest(item.filename);
    }
};
