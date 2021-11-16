#pragma once

#include <viewer/TweakableProperty.h>

#include <math/mat4.h>
#include <math/vec3.h>

#include <viewer/json.hpp>
#include <vector>
#include <string>
#include <iostream>

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

    struct RequestedTexture {
        std::string filename{};
        bool isSrgb{};
        bool isAlpha{};
        bool doRequestReload{};
    };

    json toJson();
    void fromJson(const json& source);
    void drawUI();

    const TweakableMaterial::RequestedTexture nextRequestedTexture();

    TweakablePropertyTextured<filament::math::float4> mBaseColor{ {0.0f, 0.0f, 0.0f, 1.0f} };

    TweakablePropertyTextured<float, false> mNormal{};
    TweakableProperty<float> mRoughnessScale{};
    TweakablePropertyTextured<float> mRoughness;
    TweakablePropertyTextured<float> mMetallic{};

    TweakableProperty<float> mClearCoat{};
    TweakablePropertyTextured<float> mClearCoatNormal{};
    TweakablePropertyTextured<float> mClearCoatRoughness{};

    std::vector< RequestedTexture > mRequestedTextures{};

    float mBaseTextureScale = 1.0f; // applied to baseColor texture
    float mNormalTextureScale = 1.0f; // applied to normal.xy, roughness, and metallic maps
    float mClearCoatTextureScale = 1.0f; // applied to clearcloat normal.xy, roughness, and value textures
    float mRefractiveTextureScale = 1.0f; // applied to ior, transmission, {micro}Thickness textures
    TweakableProperty<float> mSpecularIntensity{}; // scales Filament's reflectance attribute - should be equivalent to specular in Blender
    TweakableProperty<float> mNormalIntensity{}; // scales the normal vectors along the XY axes

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
    template< typename T, bool MayContainFile = false, bool IsColor = true, typename = IsValidTweakableType<T> >
    void enqueueTextureRequest(TweakableProperty<T, MayContainFile, IsColor>& item, bool isSrgb = false, bool isAlpa = false) {
        enqueueTextureRequest(item.filename, item.doRequestReload, isSrgb, isAlpa);
        item.doRequestReload = false;
    }

    void enqueueTextureRequest(const std::string& filename, bool doRequestReload, bool isSrgb = false, bool isAlpa = false);

    template< typename T, bool MayContainFile = false, bool IsColor = true, typename = IsValidTweakableType<T> >
    void writeTexturedToJson(json& result, const std::string& prefix, const TweakableProperty<T, MayContainFile, IsColor>& item) {
        result[prefix] = item.value;
        result[prefix + "IsFile"] = item.isFile;
        result[prefix + "Texture"] = item.filename;
    }

    template< typename T, bool MayContainFile = false, bool IsColor = true, typename = IsValidTweakableType<T> >
    void readTexturedFromJson(const json& source, const std::string& prefix, TweakableProperty<T, MayContainFile, IsColor>& item) {
        try {
            item.value = source[prefix];
            item.isFile = source[prefix + "IsFile"];
            item.filename = source[prefix + "Texture"];
            item.doRequestReload = true;
            if (item.isFile) enqueueTextureRequest(item.filename, item.doRequestReload);
        } catch (...) {
            std::cout << "Unable to read textured property '" << prefix << "', reverting to default value without texture." << std::endl;
            item.value = T();
            item.isFile = false;
            item.filename.clear();
        }
    }

    template< typename T, bool IsColor = false, typename = IsValidTweakableType<T> >
    void readValueFromJson(const json& source, const std::string& prefix, TweakableProperty<T, false, IsColor>& item, const T defaultValue = T()) {
        try {
            item.value = source[prefix];
        } catch (...) {
            std::cout << "Material file did not have attribute '" << prefix << "'. Using default (" << defaultValue << ") instead." << std::endl;
            item.value = defaultValue;
        }
    }

    template< typename T, bool IsColor = false, typename = IsValidTweakableType<T> >
    void readValueFromJson(const json& source, const std::string& prefix, T& item, const T defaultValue = T()) {
        try {
            item = source[prefix];
        } catch (...) {
            std::cout << "Material file did not have attribute '" << prefix << "'. Using default (" << defaultValue << ") instead." << std::endl;
            item = defaultValue;
        }
    }
};
