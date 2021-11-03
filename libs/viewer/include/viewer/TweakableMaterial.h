#pragma once

#include <viewer/TweakableProperty.h>

#include <math/mat4.h>
#include <math/vec3.h>

#include <viewer/json.hpp>

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

    TweakableMaterial() {}

    json toJson() const {
        json result{};

        result["baseColor"] = mBaseColor.value;
        result["roughness"] = mRoughness.value;
        result["clearCoat"] = mClearCoat.value;

        return result;
    }
    void fromJson(const json& source) {
        nlohmann::adl_serializer<filament::math::float4>::from_json(source["baseColor"], mBaseColor.value);

        mRoughness.value = source["roughness"];
        mClearCoat.value = source["clearCoat"];
    }
    void drawUI();

    TweakableProperty<filament::math::float4> mBaseColor{ {0.0f, 0.0f, 0.0f, 1.0f} };
    TweakableProperty<float> mRoughness{};
    TweakableProperty<float> mClearCoat{};
};
