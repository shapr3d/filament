/////////////////////////////////////////////////////////////////
// Utility functions for our material tweaking extensions
// Shapr3D
/////////////////////////////////////////////////////////////////

#pragma once

#include <imgui.h>
#include <filagui/ImGuiExtensions.h>
//#include <filagui/imfilebrowser.h>

#include <math/mat4.h>
#include <math/vec3.h>

#include <functional>
#include <string>
#include <type_traits>

//////////////////////////////////////////////////////////////////////////////////////////////////
// Templated alias to have a single validator for the types that we allow to be tweaked (float, filamenth::math floatNs, etc.)
//////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename = std::enable_if_t< std::is_same_v<float, T> || std::is_same_v<filament::math::float2, T> || std::is_same_v<filament::math::float3, T> || std::is_same_v<filament::math::float4, T> > >
using IsValidTweakableType = T;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Get how many scalars the given tweak-enabled type has
//////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename = IsValidTweakableType<T> > constexpr int dimCount() {
    return T::SIZE; 
}

template<> constexpr int dimCount<float>() { 
    return 1; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ImGui widgets require pointers to their outputs and the following function creates these
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename = IsValidTweakableType<T> >
constexpr float* getPointerTo(T& instance) { 
    return instance.v; 
}

template <> constexpr float* getPointerTo<float>(float& instance) { 
    return &instance; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// A tweakable property with its own ImGui-based UI
//////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename = IsValidTweakableType<T> >
struct TweakableProperty {
    TweakableProperty() {}
    TweakableProperty(const T& prop) : value(prop) {}

    void addWidget(const char* label, float min = 0.0f, float max = 1.0f, const char* format = "%.3f", float power = 1.0f) {
        // TODO: get rid of this and use if constexpr all the way
        static const std::vector< std::function<bool(const char*, float*, float, float, const char*, float)> > widgetFunctions{ ImGui::SliderFloat, ImGui::SliderFloat2, ImGui::SliderFloat3, ImGui::SliderFloat4 };

        //ImGui::RadioButton("Value", &sourceType, 0);
        //ImGui::RadioButton("File", &sourceType, 1);
        ImGui::LabelText(label, label);
        if (!isFile) {
            if constexpr (dimCount<T>() == 3) {
                ImGui::ColorEdit3(label, getPointerTo(value));
            }
            else if constexpr (dimCount<T>() == 4) {
                ImGui::ColorEdit4(label, getPointerTo(value));
            } else {
                widgetFunctions[dimCount<T>() - 1](label, getPointerTo(value), min, max, format, power);
            }
        }
        else {
            ImGui::InputText("Filename", &filename[0], 1024);
        }
        ImGui::SameLine();
        ImGui::Checkbox("Use texture", &isFile);
        //std::cout << "isType = " << isFile << "\n";
    }

    float* asPointer() { return getPointerTo(value); }

    T value{};
    //int sourceType{};
    bool isFile{};
    char filename[1024]{};
};