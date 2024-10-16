/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <viewer/ViewerGui.h>
#include <viewer/CustomFileDialogs.h>
#include <viewer/json.hpp>

#include <filament/RenderableManager.h>
#include <filament/Skybox.h>
#include <filament/TransformManager.h>
#include <filament/LightManager.h>
#include <filament/View.h>
#include <filament/Viewport.h>

#include <filagui/ImGuiHelper.h>

#include <utils/EntityManager.h>
#include <utils/Path.h>

#include <math/mat4.h>
#include <math/vec3.h>

#include <imgui.h>
#include <filagui/ImGuiExtensions.h>

#include "stb.h"
#include "stb_image.h"

#include <cctype>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "generated/resources/shapr_materials.h"

using namespace filagui;
using namespace filament::math;

std::string g_ArtRootPathStr {};

namespace filament {
namespace viewer {

// Taken from MeshAssimp.cpp
static int loadTexture(Engine* engine, const std::string& filePath, Texture** map,
    bool sRGB, bool hasAlpha, int numChannels, const std::string& artRootPathStr) {

    std::cout << "Loading texture \"" << filePath << "\" relative to \"" << artRootPathStr << "\"" << std::endl;

    int result = 0;

    if (!filePath.empty()) {
        utils::Path path(filePath);
        utils::Path artRootPath(artRootPathStr);
        if (path.isAbsolute() && !artRootPath.isEmpty()) {
            std::cout << "\tTexture path is absolute. Making it relative to '" << artRootPathStr << "'" << std::endl;
            path.makeRelativeTo(artRootPath);
        }
        path = artRootPath + path;

        std::cout << "\tResolved path: " << path.getPath() << std::endl;

        if (path.exists()) {
            int w, h, n;

            Texture::InternalFormat inputFormat;
            if (numChannels == 1) {
                inputFormat = Texture::InternalFormat::R8;
            }
            else {
                if (sRGB) {
                    inputFormat = hasAlpha ? Texture::InternalFormat::SRGB8_A8 : Texture::InternalFormat::SRGB8;
                }
                else {
                    inputFormat = hasAlpha ? Texture::InternalFormat::RGBA8 : Texture::InternalFormat::RGB8;
                }
            }

            Texture::Format outputFormat;
            if (numChannels == 1) {
                outputFormat = Texture::Format::R;
            } else {
                outputFormat = hasAlpha ? Texture::Format::RGBA : Texture::Format::RGB;
            }
            
            uint8_t* data = stbi_load(path.getAbsolutePath().c_str(), &w, &h, &n, numChannels);
            if (data != nullptr) {
                result = n;
                *map = Texture::Builder()
                    .width(uint32_t(w))
                    .height(uint32_t(h))
                    .levels(0xff)
                    .format(inputFormat)
                    .build(*engine);

                Texture::PixelBufferDescriptor buffer(data,
                    size_t(w * h * numChannels),
                    outputFormat,
                    Texture::Type::UBYTE,
                    (Texture::PixelBufferDescriptor::Callback)&stbi_image_free);

                (*map)->setImage(*engine, 0, std::move(buffer));
                (*map)->generateMipmaps(*engine);
            }
            else {
                std::cout << "The texture " << path << " could not be loaded" << std::endl;
            }
        }
        else {
            std::cout << "The texture " << path << " does not exist" << std::endl;
        }
    }

    return result;
}

mat4f fitIntoUnitCube(const Aabb& bounds, float zoffset) {
    float3 minpt = bounds.min;
    float3 maxpt = bounds.max;
    float maxExtent;
    maxExtent = std::max(maxpt.x - minpt.x, maxpt.y - minpt.y);
    maxExtent = std::max(maxExtent, maxpt.z - minpt.z);
    float scaleFactor = 2.0f / maxExtent;
    float3 center = (minpt + maxpt) / 2.0f;
    center.z += zoffset / scaleFactor;
    return mat4f::scaling(float3(scaleFactor)) * mat4f::translation(-center);
}

static void computeRangePlot(Settings& settings, float* rangePlot) {
    float4& ranges = settings.view.colorGrading.ranges;
    ranges.y = clamp(ranges.y, ranges.x + 1e-5f, ranges.w - 1e-5f); // darks
    ranges.z = clamp(ranges.z, ranges.x + 1e-5f, ranges.w - 1e-5f); // lights

    for (size_t i = 0; i < 1024; i++) {
        float x = i / 1024.0f;
        float s = 1.0f - smoothstep(ranges.x, ranges.y, x);
        float h = smoothstep(ranges.z, ranges.w, x);
        rangePlot[i]        = s;
        rangePlot[1024 + i] = 1.0f - s - h;
        rangePlot[2048 + i] = h;
    }
}

static void rangePlotSeriesStart(int series) {
    switch (series) {
        case 0:
            ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4) ImColor::HSV(0.4f, 0.25f, 1.0f));
            break;
        case 1:
            ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4) ImColor::HSV(0.8f, 0.25f, 1.0f));
            break;
        case 2:
            ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4) ImColor::HSV(0.17f, 0.21f, 1.0f));
            break;
    }
}

static void rangePlotSeriesEnd(int series) {
    if (series < 3) {
        ImGui::PopStyleColor();
    }
}

static float getRangePlotValue(int series, void* data, int index) {
    return ((float*) data)[series * 1024 + index];
}

inline float3 curves(float3 v, float3 shadowGamma, float3 midPoint, float3 highlightScale) {
    float3 d = 1.0f / (pow(midPoint, shadowGamma - 1.0f));
    float3 dark = pow(v, shadowGamma) * d;
    float3 light = highlightScale * (v - midPoint) + midPoint;
    return float3{
            v.r <= midPoint.r ? dark.r : light.r,
            v.g <= midPoint.g ? dark.g : light.g,
            v.b <= midPoint.b ? dark.b : light.b,
    };
}

static void computeCurvePlot(Settings& settings, float* curvePlot) {
    const auto& colorGradingOptions = settings.view.colorGrading;
    for (size_t i = 0; i < 1024; i++) {
        float3 x{i / 1024.0f * 2.0f};
        float3 y = curves(x,
                colorGradingOptions.gamma,
                colorGradingOptions.midPoint,
                colorGradingOptions.scale);
        curvePlot[i]        = y.r;
        curvePlot[1024 + i] = y.g;
        curvePlot[2048 + i] = y.b;
    }
}

static void computeToneMapPlot(ColorGradingSettings& settings, float* plot) {
    float hdrMax = 10.0f;
    ToneMapper* mapper;
    switch (settings.toneMapping) {
        case ToneMapping::LINEAR:
            mapper = new LinearToneMapper;
            break;
        case ToneMapping::ACES_LEGACY:
            mapper = new ACESLegacyToneMapper;
            break;
        case ToneMapping::ACES:
            mapper = new ACESToneMapper;
            break;
        case ToneMapping::FILMIC:
            mapper = new FilmicToneMapper;
            break;
        case ToneMapping::AGX:
            mapper = new AgxToneMapper(settings.agxToneMapper.look);
            break;
        case ToneMapping::GENERIC:
            mapper = new GenericToneMapper(
                    settings.genericToneMapper.contrast,
                    settings.genericToneMapper.midGrayIn,
                    settings.genericToneMapper.midGrayOut,
                    settings.genericToneMapper.hdrMax
            );
            hdrMax = settings.genericToneMapper.hdrMax;
            break;
        case ToneMapping::DISPLAY_RANGE:
            mapper = new DisplayRangeToneMapper;
            break;
    }

    float a = std::log10(hdrMax * 1.5f / 1e-6f);
    for (size_t i = 0; i < 1024; i++) {
        float v = i;
        float x = 1e-6f * std::pow(10.0f, a * v / 1023.0f);
        plot[i] = (*mapper)(x).r;
    }

    delete mapper;
}

static void tooltipFloat(float value) {
    if (ImGui::IsItemActive() || ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%.2f", value);
    }
}

static void pushSliderColors(float hue) {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4) ImColor::HSV(hue, 0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4) ImColor::HSV(hue, 0.6f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4) ImColor::HSV(hue, 0.7f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4) ImColor::HSV(hue, 0.9f, 0.9f));
}

static void popSliderColors() { ImGui::PopStyleColor(4); }

static void colorGradingUI(Settings& settings, float* rangePlot, float* curvePlot, float* toneMapPlot) {
    const static ImVec2 verticalSliderSize(18.0f, 160.0f);
    const static ImVec2 plotLinesSize(0.0f, 160.0f);
    const static ImVec2 plotLinesWideSize(0.0f, 120.0f);

    if (ImGui::CollapsingHeader("Color grading")) {
        ColorGradingSettings& colorGrading = settings.view.colorGrading;

        ImGui::Indent();
        ImGui::Checkbox("Enabled##colorGrading", &colorGrading.enabled);

        int quality = (int) colorGrading.quality;
        ImGui::Combo("Quality##colorGradingQuality", &quality, "Low\0Medium\0High\0Ultra\0\0");
        colorGrading.quality = (decltype(colorGrading.quality)) quality;

        int colorspace = (colorGrading.colorspace == Rec709-Linear-D65) ? 0 : 1;
        ImGui::Combo("Output color space", &colorspace, "Rec709-Linear-D65\0Rec709-sRGB-D65\0\0");
        colorGrading.colorspace = (colorspace == 0) ? Rec709-Linear-D65 : Rec709-sRGB-D65;

        int toneMapping = (int) colorGrading.toneMapping;
        ImGui::Combo("Tone-mapping", &toneMapping,
                "Linear\0ACES (legacy)\0ACES\0Filmic\0AgX\0Generic\0Display Range\0\0");
        colorGrading.toneMapping = (decltype(colorGrading.toneMapping)) toneMapping;
        if (colorGrading.toneMapping == ToneMapping::GENERIC) {
            if (ImGui::CollapsingHeader("Tonemap parameters")) {
                GenericToneMapperSettings& generic = colorGrading.genericToneMapper;
                ImGui::SliderFloat("Contrast##genericToneMapper", &generic.contrast, 1e-5f, 3.0f);
                ImGui::SliderFloat("Mid-gray in##genericToneMapper", &generic.midGrayIn, 0.0f, 1.0f);
                ImGui::SliderFloat("Mid-gray out##genericToneMapper", &generic.midGrayOut, 0.0f, 1.0f);
                ImGui::SliderFloat("HDR max", &generic.hdrMax, 1.0f, 64.0f);
            }
        }
        if (colorGrading.toneMapping == ToneMapping::AGX) {
            int agxLook = (int) colorGrading.agxToneMapper.look;
            ImGui::Combo("AgX Look", &agxLook, "None\0Punchy\0Golden\0\0");
            colorGrading.agxToneMapper.look = (decltype(colorGrading.agxToneMapper.look)) agxLook;
        }

        computeToneMapPlot(colorGrading, toneMapPlot);

        ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4) ImColor::HSV(0.17f, 0.21f, 0.9f));
        ImGui::PlotLines("", toneMapPlot, 1024, 0, "Tone map", 0.0f, 1.05f, ImVec2(0, 160));
        ImGui::PopStyleColor();

        ImGui::Checkbox("Luminance scaling", &colorGrading.luminanceScaling);
        ImGui::Checkbox("Gamut mapping", &colorGrading.gamutMapping);

        ImGui::SliderFloat("Exposure", &colorGrading.exposure, -10.0f, 10.0f);
        ImGui::SliderFloat("Night adaptation", &colorGrading.nightAdaptation, 0.0f, 1.0f);

        if (ImGui::CollapsingHeader("White balance")) {
            int temperature = colorGrading.temperature * 100.0f;
            int tint = colorGrading.tint * 100.0f;
            ImGui::SliderInt("Temperature", &temperature, -100, 100);
            ImGui::SliderInt("Tint", &tint, -100, 100);
            colorGrading.temperature = temperature / 100.0f;
            colorGrading.tint = tint / 100.0f;
        }

        if (ImGui::CollapsingHeader("Channel mixer")) {
            pushSliderColors(0.0f / 7.0f);
            ImGui::VSliderFloat("##outRed.r", verticalSliderSize, &colorGrading.outRed.r, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outRed.r);
            ImGui::SameLine();
            ImGui::VSliderFloat("##outRed.g", verticalSliderSize, &colorGrading.outRed.g, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outRed.g);
            ImGui::SameLine();
            ImGui::VSliderFloat("##outRed.b", verticalSliderSize, &colorGrading.outRed.b, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outRed.b);
            ImGui::SameLine(0.0f, 18.0f);
            popSliderColors();

            pushSliderColors(2.0f / 7.0f);
            ImGui::VSliderFloat("##outGreen.r", verticalSliderSize, &colorGrading.outGreen.r, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outGreen.r);
            ImGui::SameLine();
            ImGui::VSliderFloat("##outGreen.g", verticalSliderSize, &colorGrading.outGreen.g, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outGreen.g);
            ImGui::SameLine();
            ImGui::VSliderFloat("##outGreen.b", verticalSliderSize, &colorGrading.outGreen.b, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outGreen.b);
            ImGui::SameLine(0.0f, 18.0f);
            popSliderColors();

            pushSliderColors(4.0f / 7.0f);
            ImGui::VSliderFloat("##outBlue.r", verticalSliderSize, &colorGrading.outBlue.r, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outBlue.r);
            ImGui::SameLine();
            ImGui::VSliderFloat("##outBlue.g", verticalSliderSize, &colorGrading.outBlue.g, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outBlue.g);
            ImGui::SameLine();
            ImGui::VSliderFloat("##outBlue.b", verticalSliderSize, &colorGrading.outBlue.b, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outBlue.b);
            popSliderColors();
        }
        if (ImGui::CollapsingHeader("Tonal ranges")) {
            ImGui::ColorEdit3("Shadows", &colorGrading.shadows.x);
            ImGui::SliderFloat("Weight##shadowsWeight", &colorGrading.shadows.w, -2.0f, 2.0f);
            ImGui::ColorEdit3("Mid-tones", &colorGrading.midtones.x);
            ImGui::SliderFloat("Weight##midTonesWeight", &colorGrading.midtones.w, -2.0f, 2.0f);
            ImGui::ColorEdit3("Highlights", &colorGrading.highlights.x);
            ImGui::SliderFloat("Weight##highlightsWeight", &colorGrading.highlights.w, -2.0f, 2.0f);
            ImGui::SliderFloat4("Ranges", &colorGrading.ranges.x, 0.0f, 1.0f);
            computeRangePlot(settings, rangePlot);
            ImGuiExt::PlotLinesSeries("", 3,
                    rangePlotSeriesStart, getRangePlotValue, rangePlotSeriesEnd,
                    rangePlot, 1024, 0, "", 0.0f, 1.0f, plotLinesWideSize);
        }
        if (ImGui::CollapsingHeader("Color decision list")) {
            ImGui::SliderFloat3("Slope", &colorGrading.slope.x, 0.0f, 2.0f);
            ImGui::SliderFloat3("Offset", &colorGrading.offset.x, -0.5f, 0.5f);
            ImGui::SliderFloat3("Power", &colorGrading.power.x, 0.0f, 2.0f);
        }
        if (ImGui::CollapsingHeader("Adjustments")) {
            ImGui::SliderFloat("Contrast", &colorGrading.contrast, 0.0f, 2.0f);
            ImGui::SliderFloat("Vibrance", &colorGrading.vibrance, 0.0f, 2.0f);
            ImGui::SliderFloat("Saturation", &colorGrading.saturation, 0.0f, 2.0f);
        }
        if (ImGui::CollapsingHeader("Curves")) {
            ImGui::Checkbox("Linked curves", &colorGrading.linkedCurves);

            computeCurvePlot(settings, curvePlot);

            if (!colorGrading.linkedCurves) {
                pushSliderColors(0.0f / 7.0f);
                ImGui::VSliderFloat("##curveGamma.r", verticalSliderSize, &colorGrading.gamma.r, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.gamma.r);
                ImGui::SameLine();
                ImGui::VSliderFloat("##curveMid.r", verticalSliderSize, &colorGrading.midPoint.r, 0.0f, 2.0f, "");
                tooltipFloat(colorGrading.midPoint.r);
                ImGui::SameLine();
                ImGui::VSliderFloat("##curveScale.r", verticalSliderSize, &colorGrading.scale.r, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.scale.r);
                ImGui::SameLine(0.0f, 18.0f);
                popSliderColors();

                ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4) ImColor::HSV(0.0f, 0.7f, 0.8f));
                ImGui::PlotLines("", curvePlot, 1024, 0, "Red", 0.0f, 2.0f, plotLinesSize);
                ImGui::PopStyleColor();

                pushSliderColors(2.0f / 7.0f);
                ImGui::VSliderFloat("##curveGamma.g", verticalSliderSize, &colorGrading.gamma.g, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.gamma.g);
                ImGui::SameLine();
                ImGui::VSliderFloat("##curveMid.g", verticalSliderSize, &colorGrading.midPoint.g, 0.0f, 2.0f, "");
                tooltipFloat(colorGrading.midPoint.g);
                ImGui::SameLine();
                ImGui::VSliderFloat("##curveScale.g", verticalSliderSize, &colorGrading.scale.g, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.scale.g);
                ImGui::SameLine(0.0f, 18.0f);
                popSliderColors();

                ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4) ImColor::HSV(0.3f, 0.7f, 0.8f));
                ImGui::PlotLines("", curvePlot + 1024, 1024, 0, "Green", 0.0f, 2.0f, plotLinesSize);
                ImGui::PopStyleColor();

                pushSliderColors(4.0f / 7.0f);
                ImGui::VSliderFloat("##curveGamma.b", verticalSliderSize, &colorGrading.gamma.b, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.gamma.b);
                ImGui::SameLine();
                ImGui::VSliderFloat("##curveMid.b", verticalSliderSize, &colorGrading.midPoint.b, 0.0f, 2.0f, "");
                tooltipFloat(colorGrading.midPoint.b);
                ImGui::SameLine();
                ImGui::VSliderFloat("##curveScale.b", verticalSliderSize, &colorGrading.scale.b, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.scale.b);
                ImGui::SameLine(0.0f, 18.0f);
                popSliderColors();

                ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4) ImColor::HSV(0.6f, 0.7f, 0.8f));
                ImGui::PlotLines("", curvePlot + 2048, 1024, 0, "Blue", 0.0f, 2.0f, plotLinesSize);
                ImGui::PopStyleColor();
            } else {
                ImGui::VSliderFloat("##curveGamma", verticalSliderSize, &colorGrading.gamma.r, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.gamma.r);
                ImGui::SameLine();
                ImGui::VSliderFloat("##curveMid", verticalSliderSize, &colorGrading.midPoint.r, 0.0f, 2.0f, "");
                tooltipFloat(colorGrading.midPoint.r);
                ImGui::SameLine();
                ImGui::VSliderFloat("##curveScale", verticalSliderSize, &colorGrading.scale.r, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.scale.r);
                ImGui::SameLine(0.0f, 18.0f);

                colorGrading.gamma = float3{colorGrading.gamma.r};
                colorGrading.midPoint = float3{colorGrading.midPoint.r};
                colorGrading.scale = float3{colorGrading.scale.r};

                ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4) ImColor::HSV(0.17f, 0.21f, 0.9f));
                ImGui::PlotLines("", curvePlot, 1024, 0, "RGB", 0.0f, 2.0f, plotLinesSize);
                ImGui::PopStyleColor();
            }
        }
        ImGui::Unindent();
    }
}

ViewerGui::ViewerGui(filament::Engine* engine, filament::Scene* scene, filament::View* view,
        int sidebarWidth) :
        mEngine(engine), mScene(scene), mView(view),
        mSunlight(utils::EntityManager::get().create()),
        mSidebarWidth(sidebarWidth) {

    mSettings.view.shadowType = ShadowType::PCF;
    mSettings.view.vsmShadowOptions.anisotropy = 0;
    mSettings.view.dithering = Dithering::TEMPORAL;
    mSettings.view.antiAliasing = AntiAliasing::FXAA;
    mSettings.view.msaa = { .enabled = true, .sampleCount = 4 };
    mSettings.view.ssao.enabled = true;
    mSettings.view.bloom.enabled = true;

    using namespace filament;
    LightManager::Builder(LightManager::Type::SUN)
        .color(mSettings.lighting.sunlightColor)
        .intensity(mSettings.lighting.sunlightIntensity)
        .direction(normalize(mSettings.lighting.sunlightDirection))
        .castShadows(true)
        .sunAngularRadius(mSettings.lighting.sunlightAngularRadius)
        .sunHaloSize(mSettings.lighting.sunlightHaloSize)
        .sunHaloFalloff(mSettings.lighting.sunlightHaloFalloff)
        .build(*engine, mSunlight);
    if (mSettings.lighting.enableSunlight) {
        mScene->addEntity(mSunlight);
    }
    view->setAmbientOcclusionOptions({ .upsampling = View::QualityLevel::HIGH });

    mShaprGeneralMaterials[0] =
        Material::Builder()
        .package(SHAPR_MATERIALS_OPAQUE_DATA, SHAPR_MATERIALS_OPAQUE_SIZE)
        .build(*mEngine);
    mShaprGeneralMaterials[1] =
        Material::Builder()
        .package(SHAPR_MATERIALS_TRANSPARENT_DATA, SHAPR_MATERIALS_TRANSPARENT_SIZE)
        .build(*mEngine);
    // for legacy reasons
    mShaprGeneralMaterials[2] =
        Material::Builder()
        .package(SHAPR_MATERIALS_REFRACTIVE_DATA, SHAPR_MATERIALS_REFRACTIVE_SIZE)
        .build(*mEngine);
    mShaprGeneralMaterials[3] =
        Material::Builder()
        .package(SHAPR_MATERIALS_CLOTH_DATA, SHAPR_MATERIALS_CLOTH_SIZE)
        .build(*mEngine);
    mShaprGeneralMaterials[4] =
        Material::Builder()
        .package(SHAPR_MATERIALS_SUBSURFACE_DATA, SHAPR_MATERIALS_SUBSURFACE_SIZE)
        .build(*mEngine);
}

ViewerGui::~ViewerGui() {
    for (auto* mi : mMaterialInstances) {
        if (mi != nullptr) {
            mEngine->destroy(mi);
        }
    }
    for (auto* shaprMat : mShaprGeneralMaterials) {
        mEngine->destroy(shaprMat);
    }

    for (auto textureEntry : mTextures) {
        mEngine->destroy(textureEntry.second);
    }
    mEngine->destroy(mSunlight);
    delete mImGuiHelper;
}

void ViewerGui::setAsset(FilamentAsset* asset, FilamentInstance* instance) {
    if (mInstance != instance || mAsset != asset) {
        removeAsset();

        // We keep a non-const reference to the asset for popRenderables and getWireframe.
        mAsset = asset;
        mInstance = instance;

        mVisibleScenes.reset();
        mVisibleScenes.set(0);
        mCurrentCamera = 0;
        if (!mAsset) {
            mAsset = nullptr;
            mInstance = nullptr;
            return;
        }
        updateRootTransform();
        mScene->addEntities(asset->getLightEntities(), asset->getLightEntityCount());
        auto& rcm = mEngine->getRenderableManager();
        for (size_t i = 0, n = asset->getRenderableEntityCount(); i < n; i++) {
            auto ri = rcm.getInstance(asset->getRenderableEntities()[i]);
            rcm.setScreenSpaceContactShadows(ri, true);
        }
    }
}

void ViewerGui::populateScene() {
    static constexpr int kNumAvailable = 128;
    utils::Entity renderables[kNumAvailable];
    while (size_t numWritten = mAsset->popRenderables(renderables, kNumAvailable)) {
        mAsset->addEntitiesToScene(*mScene, renderables, numWritten, mVisibleScenes);
    }
}

void ViewerGui::removeAsset() {
    if (!isRemoteMode()) {
        mScene->removeEntities(mAsset->getEntities(), mAsset->getEntityCount());
        mAsset = nullptr;
    }
}

UTILS_NOINLINE
static bool notequal(float a, float b) noexcept {
    return a != b;
}

// we do this to circumvent -ffast-math ignoring NaNs
static bool is_not_a_number(float v) noexcept {
    return notequal(v, v);
}

void ViewerGui::setIndirectLight(filament::IndirectLight* ibl,
        filament::math::float3 const* sh3) {
    using namespace filament::math;

    mIbl = ibl;
    mSh3 = sh3;

    mSettings.view.fog.color = sh3[0];
    mIndirectLight = ibl;
}

void ViewerGui::setSunlightFromIbl() {
    using namespace filament::math;
    if (mIbl && mSh3) {
        float3 const d = filament::IndirectLight::getDirectionEstimate(mSh3);
        float4 const c = filament::IndirectLight::getColorEstimate(mSh3, d);
        bool const dIsValid = std::none_of(std::begin(d.v), std::end(d.v), is_not_a_number);
        bool const cIsValid = std::none_of(std::begin(c.v), std::end(c.v), is_not_a_number);
        if (dIsValid && cIsValid) {
            mSettings.lighting.sunlightDirection = mIbl->getRotation() * d;
            mSettings.lighting.sunlightColor = c.rgb;
            mSettings.lighting.sunlightIntensity = c[3] * mIbl->getIntensity();
        }
    }
}

void ViewerGui::updateRootTransform() {
    if (isRemoteMode()) {
        return;
    }
    auto& tcm = mEngine->getTransformManager();
    auto root = tcm.getInstance(mAsset->getRoot());
    filament::math::mat4f transform;
    if (mSettings.viewer.autoScaleEnabled) {
        FilamentInstance* instance = mAsset->getInstance();
        Aabb aabb = instance ? instance->getBoundingBox() : mAsset->getBoundingBox();
        transform = fitIntoUnitCube(aabb, 4);
    }
    transform *= filament::math::mat4f::rotation(filament::math::F_PI_2, filament::math::vec3<float>(-1, 0, 0));
    tcm.setTransform(root, transform);
}

void ViewerGui::sceneSelectionUI() {
    // Build a list of checkboxes, one for each glTF scene.
    bool changed = false;
    for (size_t i = 0, n = mAsset->getSceneCount(); i < n; ++i) {
        bool isVisible = mVisibleScenes.test(i);
        const char* name = mAsset->getSceneName(i);
        if (name) {
            changed = ImGui::Checkbox(name, &isVisible) || changed;
        } else {
            char label[16];
            snprintf(label, 16, "Scene %zu", i);
            changed = ImGui::Checkbox(label, &isVisible) || changed;
        }
        if (isVisible) {
            mVisibleScenes.set(i);
        } else {
            mVisibleScenes.unset(i);
        }
    }
    // If any checkboxes have been toggled, rebuild the scene list.
    if (changed) {
        const utils::Entity* entities = mAsset->getRenderableEntities();
        const size_t entityCount = mAsset->getRenderableEntityCount();
        mScene->removeEntities(entities, entityCount);
        mAsset->addEntitiesToScene(*mScene, entities, entityCount, mVisibleScenes);
    }
}

void ViewerGui::applyAnimation(double currentTime, FilamentInstance* instance) {
    instance = instance ? instance : mInstance;
    assert_invariant(!isRemoteMode());
    if (instance == nullptr) {
        return;
    }
    Animator& animator = *instance->getAnimator();
    const size_t animationCount = animator.getAnimationCount();
    if (mResetAnimation) {
        mPreviousStartTime = mCurrentStartTime;
        mCurrentStartTime = currentTime;
        mResetAnimation = false;
    }
    const double elapsedSeconds = currentTime - mCurrentStartTime;
    if (animationCount > 0 && mCurrentAnimation >= 0) {
        if (mCurrentAnimation == animationCount) {
            for (size_t i = 0; i < animationCount; i++) {
                animator.applyAnimation(i, elapsedSeconds);
            }
        } else {
            animator.applyAnimation(mCurrentAnimation, elapsedSeconds);
        }
        if (elapsedSeconds < mCrossFadeDuration && mPreviousAnimation >= 0 && mPreviousAnimation != animationCount) {
            const double previousSeconds = currentTime - mPreviousStartTime;
            const float lerpFactor = elapsedSeconds / mCrossFadeDuration;
            animator.applyCrossFade(mPreviousAnimation, previousSeconds, lerpFactor);
        }
    }
    if (mShowingRestPose) {
        animator.resetBoneMatrices();
    } else {
        animator.updateBoneMatrices();
    }
}

void ViewerGui::renderUserInterface(float timeStepInSeconds, View* guiView, float pixelRatio) {
    if (mImGuiHelper == nullptr) {
        mImGuiHelper = new ImGuiHelper(mEngine, guiView, "");

        auto& io = ImGui::GetIO();

        // The following table uses normal ANSI codes, which is consistent with the keyCode that
        // comes from a web "keydown" event.
        io.KeyMap[ImGuiKey_Tab] = 9;
        io.KeyMap[ImGuiKey_LeftArrow] = 37;
        io.KeyMap[ImGuiKey_RightArrow] = 39;
        io.KeyMap[ImGuiKey_UpArrow] = 38;
        io.KeyMap[ImGuiKey_DownArrow] = 40;
        io.KeyMap[ImGuiKey_Home] = 36;
        io.KeyMap[ImGuiKey_End] = 35;
        io.KeyMap[ImGuiKey_Delete] = 46;
        io.KeyMap[ImGuiKey_Backspace] = 8;
        io.KeyMap[ImGuiKey_Enter] = 13;
        io.KeyMap[ImGuiKey_Escape] = 27;
        io.KeyMap[ImGuiKey_A] = 65;
        io.KeyMap[ImGuiKey_C] = 67;
        io.KeyMap[ImGuiKey_V] = 86;
        io.KeyMap[ImGuiKey_X] = 88;
        io.KeyMap[ImGuiKey_Y] = 89;
        io.KeyMap[ImGuiKey_Z] = 90;

        // TODO: this is not the best way to handle high DPI in ImGui, but it is fine when using the
        // proggy font. Users need to refresh their window when dragging between displays with
        // different pixel ratios.
        io.FontGlobalScale = pixelRatio;
        ImGui::GetStyle().ScaleAllSizes(pixelRatio);
    }

    const auto size = guiView->getViewport();
    mImGuiHelper->setDisplaySize(size.width, size.height, 1, 1);

    mImGuiHelper->render(timeStepInSeconds, [this](Engine*, View*) {
        this->updateUserInterface();
    });
}

void ViewerGui::mouseEvent(float mouseX, float mouseY, bool mouseButton, float mouseWheelY,
        bool control) {
    if (mImGuiHelper) {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos.x = mouseX;
        io.MousePos.y = mouseY;
        io.MouseWheel += mouseWheelY;
        io.MouseDown[0] = mouseButton != 0;
        io.MouseDown[1] = false;
        io.MouseDown[2] = false;
        io.KeyCtrl = control;
    }
}

void ViewerGui::keyDownEvent(int keyCode) {
    if (mImGuiHelper && keyCode < IM_ARRAYSIZE(ImGui::GetIO().KeysDown)) {
        ImGui::GetIO().KeysDown[keyCode] = true;
    }
}

void ViewerGui::keyUpEvent(int keyCode) {
    if (mImGuiHelper && keyCode < IM_ARRAYSIZE(ImGui::GetIO().KeysDown)) {
        ImGui::GetIO().KeysDown[keyCode] = false;
    }
}

void ViewerGui::keyPressEvent(int charCode) {
    if (mImGuiHelper) {
        ImGui::GetIO().AddInputCharacter(charCode);
    }
}

void ViewerGui::saveTweaksToFile(TweakableMaterial* tweaks, const char* filePath) {
    nlohmann::json js = tweaks->toJson();
    std::string filePathStr = filePath;
    if (filePathStr.rfind(".json") != (filePathStr.size() - 5)) {
        filePathStr += ".json";
    }
    mLastSavedFileName = filePathStr;
    std::fstream outFile(filePathStr, std::ios::binary | std::ios::out);
    outFile << std::setw(4) << js << std::endl;
    outFile.close();
}


void ViewerGui::loadTweaksFromFile(const std::string& entityName, const std::string& filePath) {
    auto entityMaterial = mTweakedMaterials.find(entityName);
    if (entityMaterial == mTweakedMaterials.end()) {
        std::cout << "Cannot quick load material '" << filePath << "' for entity '" << entityName << "'" << std::endl;
        return;
    }
    TweakableMaterial& tweaks = entityMaterial->second;

    std::fstream inFile(filePath, std::ios::binary | std::ios::in);
    nlohmann::json js{};
    inFile >> js;
    inFile.close();
    tweaks.fromJson(js);
}

void ViewerGui::changeElementVisibility(utils::Entity entity, int elementIndex, bool newVisibility) {
    // The children of the root are the below-Node level nodes in the graph - indexing refers to them by artist request
    auto& tm = mEngine->getTransformManager();
    auto& rm = mEngine->getRenderableManager();

    auto rinstance = rm.getInstance(entity);
    auto tinstance = tm.getInstance(entity);

    auto getNthChild = [&](utils::Entity& parentEntity, int N = 0) {
        auto tparentEntity = tm.getInstance(parentEntity);
        utils::Entity nthChild;
        if (N < tm.getChildCount(tparentEntity)) {
            tm.getChildren(tparentEntity, &nthChild, N);
        }
        return nthChild;
    };

    auto hasChild = [&](utils::Entity& parentEntity) {
        auto tparentEntity = tm.getInstance(parentEntity);
        return tm.getChildCount(tparentEntity) != 0;
    };

    if (hasChild(entity)) {
        auto tparentOfRoi = tm.getInstance(entity);
        std::vector<utils::Entity> children(tm.getChildCount(tparentOfRoi));
        tm.getChildren(tinstance, children.data(), children.size());

        if (elementIndex < children.size()) {
            changeAllVisibility(children[elementIndex], newVisibility);
        }
    }
}

void ViewerGui::changeAllVisibility(utils::Entity entity, bool changeToVisible) {
    auto& tm = mEngine->getTransformManager();
    if (!changeToVisible) {
        mScene->remove(entity);
    }
    else {
        mScene->addEntity(entity);
    }

    auto tinstance = tm.getInstance(entity);

    std::vector<utils::Entity> children(tm.getChildCount(tinstance));

    tm.getChildren(tinstance, children.data(), children.size());
    for (auto ce : children) {
        changeAllVisibility(ce, changeToVisible);
    }
};

const char* ViewerGui::formatToName(filament::Texture::InternalFormat format) const {
    switch (format) {
    case filament::Texture::InternalFormat::R8: return "R8";
    case filament::Texture::InternalFormat::R8_SNORM: return "R8_SNORM";
    case filament::Texture::InternalFormat::R8UI: return "R8UI";
    case filament::Texture::InternalFormat::R8I: return "R8I";
    case filament::Texture::InternalFormat::STENCIL8: return "STENCIL8";
    case filament::Texture::InternalFormat::R16F: return "R16F";
    case filament::Texture::InternalFormat::R16UI: return "R16UI";
    case filament::Texture::InternalFormat::R16I: return "R16I";
    case filament::Texture::InternalFormat::RG8: return "RG8";
    case filament::Texture::InternalFormat::RG8_SNORM: return "RG8_SNORM";
    case filament::Texture::InternalFormat::RG8UI: return "RG8UI";
    case filament::Texture::InternalFormat::RG8I: return "RG8I";
    case filament::Texture::InternalFormat::RGB565: return "RGB565";
    case filament::Texture::InternalFormat::RGB9_E5: return "RGB9_E5";
    case filament::Texture::InternalFormat::RGB5_A1: return "RGB5_A1";
    case filament::Texture::InternalFormat::RGBA4: return "RGBA4";
    case filament::Texture::InternalFormat::DEPTH16: return "DEPTH16";
    case filament::Texture::InternalFormat::RGB8: return "RGB8";
    case filament::Texture::InternalFormat::SRGB8: return "SRGB8";
    case filament::Texture::InternalFormat::RGB8_SNORM: return "RGB8_SNORM";
    case filament::Texture::InternalFormat::RGB8UI: return "RGB8UI";
    case filament::Texture::InternalFormat::RGB8I: return "RGB8I";
    case filament::Texture::InternalFormat::DEPTH24: return "DEPTH24";
    case filament::Texture::InternalFormat::R32F: return "R32F";
    case filament::Texture::InternalFormat::R32UI: return "R32UI";
    case filament::Texture::InternalFormat::R32I: return "R32I";
    case filament::Texture::InternalFormat::RG16F: return "RG16F";
    case filament::Texture::InternalFormat::RG16UI: return "RG16UI";
    case filament::Texture::InternalFormat::RG16I: return "RG16I";
    case filament::Texture::InternalFormat::R11F_G11F_B10F: return "R11F_G11F_B10F";
    case filament::Texture::InternalFormat::RGBA8: return "RGBA8";
    case filament::Texture::InternalFormat::SRGB8_A8: return "SRGB8_A8";
    case filament::Texture::InternalFormat::RGBA8_SNORM: return "RGBA8_SNORM";
    case filament::Texture::InternalFormat::UNUSED: return "UNUSED";
    case filament::Texture::InternalFormat::RGB10_A2: return "RGB10_A2";
    case filament::Texture::InternalFormat::RGBA8UI: return "RGBA8UI";
    case filament::Texture::InternalFormat::RGBA8I: return "RGBA8I";
    case filament::Texture::InternalFormat::DEPTH32F: return "DEPTH32F";
    case filament::Texture::InternalFormat::DEPTH24_STENCIL8: return "DEPTH24_STENCIL8";
    case filament::Texture::InternalFormat::DEPTH32F_STENCIL8: return "DEPTH32F_STENCIL8";
    case filament::Texture::InternalFormat::RGB16F: return "RGB16F";
    case filament::Texture::InternalFormat::RGB16UI: return "RGB16UI";
    case filament::Texture::InternalFormat::RGB16I: return "RGB16I";
    case filament::Texture::InternalFormat::RG32F: return "RG32F";
    case filament::Texture::InternalFormat::RG32UI: return "RG32UI";
    case filament::Texture::InternalFormat::RG32I: return "RG32I";
    case filament::Texture::InternalFormat::RGBA16F: return "RGBA16F";
    case filament::Texture::InternalFormat::RGBA16UI: return "RGBA16UI";
    case filament::Texture::InternalFormat::RGBA16I: return "RGBA16I";
    case filament::Texture::InternalFormat::RGB32F: return "RGB32F";
    case filament::Texture::InternalFormat::RGB32UI: return "RGB32UI";
    case filament::Texture::InternalFormat::RGB32I: return "RGB32I";
    case filament::Texture::InternalFormat::RGBA32F: return "RGBA32F";
    case filament::Texture::InternalFormat::RGBA32UI: return "RGBA32UI";
    case filament::Texture::InternalFormat::RGBA32I: return "RGBA32I";
    case filament::Texture::InternalFormat::EAC_R11: return "EAC_R11";
    case filament::Texture::InternalFormat::EAC_R11_SIGNED: return "EAC_R11_SIGNED";
    case filament::Texture::InternalFormat::EAC_RG11: return "EAC_RG11";
    case filament::Texture::InternalFormat::EAC_RG11_SIGNED: return "EAC_RG11_SIGNED";
    case filament::Texture::InternalFormat::ETC2_RGB8: return "ETC2_RGB8";
    case filament::Texture::InternalFormat::ETC2_SRGB8: return "ETC2_SRGB8";
    case filament::Texture::InternalFormat::ETC2_RGB8_A1: return "ETC2_RGB8_A1";
    case filament::Texture::InternalFormat::ETC2_SRGB8_A1: return "ETC2_SRGB8_A1";
    case filament::Texture::InternalFormat::ETC2_EAC_RGBA8: return "ETC2_EAC_RGBA8";
    case filament::Texture::InternalFormat::ETC2_EAC_SRGBA8: return "ETC2_EAC_SRGBA8";
    case filament::Texture::InternalFormat::DXT1_RGB: return "DXT1_RGB";
    case filament::Texture::InternalFormat::DXT1_RGBA: return "DXT1_RGBA";
    case filament::Texture::InternalFormat::DXT3_RGBA: return "DXT3_RGBA";
    case filament::Texture::InternalFormat::DXT5_RGBA: return "DXT5_RGBA";
    case filament::Texture::InternalFormat::DXT1_SRGB: return "DXT1_SRGB";
    case filament::Texture::InternalFormat::DXT1_SRGBA: return "DXT1_SRGBA";
    case filament::Texture::InternalFormat::DXT3_SRGBA: return "DXT3_SRGBA";
    case filament::Texture::InternalFormat::DXT5_SRGBA: return "DXT5_SRGBA";
    case filament::Texture::InternalFormat::RED_RGTC1: return "RED_RGTC1";
    case filament::Texture::InternalFormat::SIGNED_RED_RGTC1: return "SIGNED_RED_RGTC1";
    case filament::Texture::InternalFormat::RED_GREEN_RGTC2: return "RED_GREEN_RGTC2";
    case filament::Texture::InternalFormat::SIGNED_RED_GREEN_RGTC2: return "SIGNED_RED_GREEN_RGTC2";
    case filament::Texture::InternalFormat::RGB_BPTC_SIGNED_FLOAT: return "RGB_BPTC_SIGNED_FLOAT";
    case filament::Texture::InternalFormat::RGB_BPTC_UNSIGNED_FLOAT: return "RGB_BPTC_UNSIGNED_FLOAT";
    case filament::Texture::InternalFormat::RGBA_BPTC_UNORM: return "RGBA_BPTC_UNORM";
    case filament::Texture::InternalFormat::SRGB_ALPHA_BPTC_UNORM: return "SRGB_ALPHA_BPTC_UNORM";
    case filament::Texture::InternalFormat::RGBA_ASTC_4x4: return "RGBA_ASTC_4x4";
    case filament::Texture::InternalFormat::RGBA_ASTC_5x4: return "RGBA_ASTC_5x4";
    case filament::Texture::InternalFormat::RGBA_ASTC_5x5: return "RGBA_ASTC_5x5";
    case filament::Texture::InternalFormat::RGBA_ASTC_6x5: return "RGBA_ASTC_6x5";
    case filament::Texture::InternalFormat::RGBA_ASTC_6x6: return "RGBA_ASTC_6x6";
    case filament::Texture::InternalFormat::RGBA_ASTC_8x5: return "RGBA_ASTC_8x5";
    case filament::Texture::InternalFormat::RGBA_ASTC_8x6: return "RGBA_ASTC_8x6";
    case filament::Texture::InternalFormat::RGBA_ASTC_8x8: return "RGBA_ASTC_8x8";
    case filament::Texture::InternalFormat::RGBA_ASTC_10x5: return "RGBA_ASTC_10x5";
    case filament::Texture::InternalFormat::RGBA_ASTC_10x6: return "RGBA_ASTC_10x6";
    case filament::Texture::InternalFormat::RGBA_ASTC_10x8: return "RGBA_ASTC_10x8";
    case filament::Texture::InternalFormat::RGBA_ASTC_10x10: return "RGBA_ASTC_10x10";
    case filament::Texture::InternalFormat::RGBA_ASTC_12x10: return "RGBA_ASTC_12x10";
    case filament::Texture::InternalFormat::RGBA_ASTC_12x12: return "RGBA_ASTC_12x12";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_4x4: return "SRGB8_ALPHA8_ASTC_4x4";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_5x4: return "SRGB8_ALPHA8_ASTC_5x4";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_5x5: return "SRGB8_ALPHA8_ASTC_5x5";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_6x5: return "SRGB8_ALPHA8_ASTC_6x5";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_6x6: return "SRGB8_ALPHA8_ASTC_6x6";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_8x5: return "SRGB8_ALPHA8_ASTC_8x5";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_8x6: return "SRGB8_ALPHA8_ASTC_8x6";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_8x8: return "SRGB8_ALPHA8_ASTC_8x8";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_10x5: return "SRGB8_ALPHA8_ASTC_10x5";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_10x6: return "SRGB8_ALPHA8_ASTC_10x6";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_10x8: return "SRGB8_ALPHA8_ASTC_10x8";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_10x10: return "SRGB8_ALPHA8_ASTC_10x10";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_12x10: return "SRGB8_ALPHA8_ASTC_12x10";
    case filament::Texture::InternalFormat::SRGB8_ALPHA8_ASTC_12x12: return "SRGB8_ALPHA8_ASTC_12x12";
    }
}

std::string ViewerGui::validateTweaks(const TweakableMaterial& tweaks) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++20-extensions"
    auto verifyTextured = [&]<typename T, bool MayContainFile, bool IsColor, bool IsDerivable>(
        std::string & result,
        const std::string & prompt,
        const TweakableProperty<T, MayContainFile, IsColor, IsDerivable>&prop,
        filament::Texture::InternalFormat expectedFormat = filament::Texture::InternalFormat::UNUSED) {

        if (!prop.isFile) return;

        if (prop.filename.getFileExtension() != "png") {
            result += "ERROR: " + prompt + " is not a png.\n";
        }

        auto textureEntry = mTextures.find(prop.filename.asString());
        if (textureEntry == mTextures.end()) {
            result += "ERROR: " + prompt + " cannot be found.\n";
            return;
        }
        int expectedChannelCount = 0;
        // Infer the expected texture format if no explicit cue was given by the caller
        if (expectedFormat == filament::Texture::InternalFormat::UNUSED) {
            if (IsColor) {
                if (tweaks.mShaderType == TweakableMaterial::MaterialType::Transparent || tweaks.mShaderType == TweakableMaterial::MaterialType::Refractive) {
                    expectedFormat = filament::Texture::InternalFormat::SRGB8_A8;
                    expectedChannelCount = 4;
                }
                else {
                    expectedFormat = filament::Texture::InternalFormat::SRGB8;
                    expectedChannelCount = 3;
                }
            }
            else {
                expectedFormat = filament::Texture::InternalFormat::R8;
                expectedChannelCount = 1;
            }
        }
        else {
            if (expectedFormat == filament::Texture::InternalFormat::SRGB8_A8) expectedChannelCount = 4;
            else if (expectedFormat == filament::Texture::InternalFormat::SRGB8 || expectedFormat == filament::Texture::InternalFormat::RGB8) expectedChannelCount = 3;
            else if (expectedFormat == filament::Texture::InternalFormat::R8) expectedChannelCount = 1;
        }

        if (textureEntry->second == nullptr) {
            result += "ERROR: texture for " + prompt + " could not be loaded!\n";
        } else if (textureEntry->second->getFormat() != expectedFormat) {
            result += "ERROR: " + prompt + " has incorrect format! Expected " + formatToName(expectedFormat) + ", got " + formatToName(textureEntry->second->getFormat()) + ".\n";
        }

        auto textureChannelCountEntry = mTextureFileChannels.find(prop.filename.asString());
        if (textureChannelCountEntry != mTextureFileChannels.end()) {
            if (textureChannelCountEntry->second != expectedChannelCount) {
                result += "ERROR: " + prompt + " has incorrect number of channels! Expected " + std::to_string(expectedChannelCount) + ", got " + std::to_string(textureChannelCountEntry->second) + ".\n";
            }
        }
        else {
            result += "ERROR: " + prompt + " has no channel numbers! Expected a " + std::to_string(expectedChannelCount) + " channel texture.\n";
        }
    };
#pragma clang diagnostic pop

    std::string result = "";
    verifyTextured(result, "BaseColor", tweaks.mBaseColor);

    verifyTextured(result, "Normal", tweaks.mNormal, filament::Texture::InternalFormat::RGB8);
    verifyTextured(result, "Occlusion", tweaks.mOcclusion);
    verifyTextured(result, "Roughness", tweaks.mRoughness);
    verifyTextured(result, "Metallic", tweaks.mMetallic);

    verifyTextured(result, "ClearCoat Normal", tweaks.mClearCoatNormal, filament::Texture::InternalFormat::RGB8);
    verifyTextured(result, "ClearCoat Roughness", tweaks.mClearCoatRoughness);

    verifyTextured(result, "Sheen Roughness", tweaks.mSheenRoughness);
    verifyTextured(result, "Transmission", tweaks.mTransmission);
    verifyTextured(result, "Thickness", tweaks.mThickness);

    return result;
}

void ViewerGui::updateUserInterface() {
    using namespace filament;

    static char fileDialogPath[1024];
    static const char* materialFileFilter = "JSON\0*.JSON\0";

    auto& tm = mEngine->getTransformManager();
    auto& rm = mEngine->getRenderableManager();
    auto& lm = mEngine->getLightManager();

    // Show a common set of UI widgets for all renderables.
    auto renderableTreeItem = [this, &rm](utils::Entity entity) {
        bool rvis = mScene->hasEntity(entity);
        ImGui::Checkbox("visible", &rvis);
        if (rvis) {
            mScene->addEntity(entity);
        } else {
            mScene->remove(entity);
        }
        auto instance = rm.getInstance(entity);
        bool scaster = rm.isShadowCaster(instance);
        bool sreceiver = rm.isShadowReceiver(instance);
        ImGui::Checkbox("casts shadows", &scaster);
        rm.setCastShadows(instance, scaster);
        ImGui::Checkbox("receives shadows", &sreceiver);
        rm.setReceiveShadows(instance, sreceiver);
        auto numMorphTargets = rm.getMorphTargetCount(instance);
        if (numMorphTargets > 0) {
            bool selected = entity == mCurrentMorphingEntity;
            ImGui::Checkbox("morphing", &selected);
            if (selected) {
                mCurrentMorphingEntity = entity;
                mMorphWeights.resize(numMorphTargets, 0.0f);
            } else {
                mCurrentMorphingEntity = utils::Entity();
            }
        }
        size_t numPrims = rm.getPrimitiveCount(instance);
        for (size_t prim = 0; prim < numPrims; ++prim) {
            // check if we have already assigned a custom material tweak to this entity
            const char* entityName = mAsset->getName(entity);
            auto entityMaterial = mTweakedMaterials.find(entityName);
            if (entityMaterial == mTweakedMaterials.end()) {
                // Only allow adding a new material, nothing else
                if (ImGui::Button("Add custom material")) {                
                    mTweakedMaterials.emplace(std::pair<std::string, TweakableMaterial>(entityName, TweakableMaterial{}));
                    filament::MaterialInstance* newInstance = mShaprGeneralMaterials[0]->createInstance();
                    mMaterialInstances.push_back(newInstance);
                    rm.setMaterialInstanceAt(instance, prim, newInstance);
                }
            } else {
                // As soon as they have assigned a material to it, we display its settings all the time
                if (ImGui::CollapsingHeader("Custom persistent material tweaks")) {
                    TweakableMaterial& tweaks = entityMaterial->second;

                    if (ImGui::Button("Load material")) {

                        if (SD_OpenFileDialog(fileDialogPath, materialFileFilter)) {
                            TweakableMaterial::MaterialType prevType = tweaks.mShaderType;

                            loadTweaksFromFile(entityName, fileDialogPath);

                            mLastSavedEntityName = entityName;
                            mLastSavedFileName = fileDialogPath;

                            if (tweaks.mShaderType != prevType) {
                                filament::MaterialInstance* currentInstance = rm.getMaterialInstanceAt(instance, prim);
                                for (auto& mat : mMaterialInstances) if (mat == currentInstance) mat = nullptr;
                                mEngine->destroy(currentInstance);

                                filament::MaterialInstance* newInstance = mShaprGeneralMaterials[tweaks.mShaderType]->createInstance();
                                mMaterialInstances.push_back(newInstance);
                                rm.setMaterialInstanceAt(instance, prim, newInstance);
                            }
                            tweaks.mDoesRequireValidation = true;
                        }

                    } else {
                        ImGui::SameLine();
                        if (ImGui::Button("Save material")) {
                            if (SD_SaveFileDialog(fileDialogPath, materialFileFilter)) {
                                mLastSavedEntityName = entityName;
                                saveTweaksToFile(&tweaks, fileDialogPath);
                            }
                        } else {
                            ImGui::SameLine();
                            if (ImGui::Button("Reset")) {
                                TweakableMaterial::MaterialType prevType = tweaks.mShaderType;
                                tweaks.resetWithType(prevType);
                                tweaks.mDoesRequireValidation = true;
                            }

                            std::function<void( TweakableMaterial::MaterialType materialType)> changeMaterialTypeTo;
                            changeMaterialTypeTo = [&]( TweakableMaterial::MaterialType materialType) {
                                if (tweaks.mShaderType != materialType) {
                                    filament::MaterialInstance* currentInstance = rm.getMaterialInstanceAt(instance, prim);
                                    for (auto& mat : mMaterialInstances) if (mat == currentInstance) mat = nullptr;
                                    mEngine->destroy(currentInstance);

                                    filament::MaterialInstance* newInstance = mShaprGeneralMaterials[materialType]->createInstance();
                                    mMaterialInstances.push_back(newInstance);
                                    rm.setMaterialInstanceAt(instance, prim, newInstance);
                                }
                                tweaks.mShaderType = materialType;
                                tweaks.mDoesRequireValidation = true;
                            };

                            if (ImGui::RadioButton("Opaque", tweaks.mShaderType == TweakableMaterial::MaterialType::Opaque)) {
                                changeMaterialTypeTo(TweakableMaterial::MaterialType::Opaque);
                            }
                            ImGui::SameLine();
                            if (ImGui::RadioButton("Transparent", tweaks.mShaderType == TweakableMaterial::MaterialType::Transparent)) {
                                changeMaterialTypeTo(TweakableMaterial::MaterialType::Transparent);
                            }
                            ImGui::SameLine();
                            if (ImGui::RadioButton("Refractive", tweaks.mShaderType == TweakableMaterial::MaterialType::Refractive)) {
                                changeMaterialTypeTo(TweakableMaterial::MaterialType::Refractive);
                            }
                            ImGui::SameLine();
                            if (ImGui::RadioButton("Cloth", tweaks.mShaderType == TweakableMaterial::MaterialType::Cloth)) {
                                changeMaterialTypeTo(TweakableMaterial::MaterialType::Cloth);
                            }
                            ImGui::SameLine();
                            if (ImGui::RadioButton("Subsurface", tweaks.mShaderType == TweakableMaterial::MaterialType::Subsurface)) {
                                changeMaterialTypeTo(TweakableMaterial::MaterialType::Subsurface);
                            }
                        }
                    }

                    static std::string validationResults = "";
                    tweaks.drawUI(validationResults);

                    auto checkAndFixPathRelative([](auto& path) {
                        utils::Path asPath(path);
                        if (asPath.isAbsolute()) {
                            std::string newFilePath = asPath.makeRelativeTo(g_ArtRootPathStr).c_str();
                            //std::cout << "\t\tMaking path relative: " << path << std::endl;
                            //std::cout << "\t\tMade path relative: " << newFilePath << std::endl;
                            path = newFilePath;
                        }
                    });

                    // Load the requested textures
                    bool isNewTextureLoaded = false;
                    TweakableMaterial::RequestedTexture currentRequestedTexture = tweaks.nextRequestedTexture();
                    while (currentRequestedTexture.filename != "") {
                        checkAndFixPathRelative(currentRequestedTexture.filename);
                        std::string keyName = currentRequestedTexture.filename;
                        auto textureEntry = mTextures.find(keyName);
                        if (textureEntry == mTextures.end() ) {
                            mTextures[keyName] = nullptr;
                            mTextureFileChannels[keyName] = loadTexture(mEngine, currentRequestedTexture.filename, &mTextures[keyName], currentRequestedTexture.isSrgb, currentRequestedTexture.isAlpha, currentRequestedTexture.channelCount, mSettings.viewer.artRootPath);
                            isNewTextureLoaded = true;
                        } else if (currentRequestedTexture.doRequestReload) {
                            if (mTextures[keyName] != nullptr) mEngine->destroy(mTextures[keyName]);
                            mTextureFileChannels[keyName] = loadTexture(mEngine, currentRequestedTexture.filename, &mTextures[keyName], currentRequestedTexture.isSrgb, currentRequestedTexture.isAlpha, currentRequestedTexture.channelCount, mSettings.viewer.artRootPath);
                            isNewTextureLoaded = true;
                        }

                        currentRequestedTexture = tweaks.nextRequestedTexture();
                    }

                    if (tweaks.mDoesRequireValidation || isNewTextureLoaded) {
                        tweaks.mDoesRequireValidation = false;
                        validationResults = validateTweaks(tweaks);
                        if (validationResults.length() != 0) {
                            tweaks.mDoRelease = false;
                        }
                    }
                }
            }

            const auto& matInstance = rm.getMaterialInstanceAt(instance, prim);
            const char* mname = matInstance->getName();
            const auto* mat = matInstance->getMaterial();

            if (mname) {
                ImGui::Text("prim %zu: material %s", prim, mname);
            } else {
                ImGui::Text("prim %zu: (unnamed material)", prim);
            }
        }
    };

    auto lightTreeItem = [this, &lm](utils::Entity entity) {
        bool lvis = mScene->hasEntity(entity);
        ImGui::Checkbox("visible", &lvis);

        if (lvis) {
            mScene->addEntity(entity);
        } else {
            mScene->remove(entity);
        }

        auto instance = lm.getInstance(entity);
        bool lcaster = lm.isShadowCaster(instance);
        ImGui::Checkbox("shadow caster", &lcaster);
        lm.setShadowCaster(instance, lcaster);
    };

    // Declare a std::function for tree nodes, it's an easy way to make a recursive lambda.
    std::function<void(utils::Entity)> treeNode;

    treeNode = [&](utils::Entity entity) {
        auto tinstance = tm.getInstance(entity);
        auto rinstance = rm.getInstance(entity);
        auto linstance = lm.getInstance(entity);
        intptr_t treeNodeId = 1 + entity.getId();

        const char* name = mAsset->getName(entity);
        auto getLabel = [&name, &rinstance, &linstance]() {
            if (name) {
                return name;
            }
            if (rinstance) {
                return "Mesh";
            }
            if (linstance) {
                return "Light";
            }
            return "Node";
        };
        const char* label = getLabel();

        ImGuiTreeNodeFlags flags = 0; // rinstance ? 0 : ImGuiTreeNodeFlags_DefaultOpen;
        std::vector<utils::Entity> children(tm.getChildCount(tinstance));
        if (ImGui::TreeNodeEx((const void*) treeNodeId, flags, "%s", label)) {
            if (rinstance) {
                renderableTreeItem(entity);
            }
            if (linstance) {
                lightTreeItem(entity);
            }
            tm.getChildren(tinstance, children.data(), children.size());
            for (auto ce : children) {
                treeNode(ce);
            }
            ImGui::TreePop();
        }
    };

    std::function<void(utils::Entity)> applyMaterialSettingsInTree;
    applyMaterialSettingsInTree = [&](utils::Entity entity) {
        auto tinstance = tm.getInstance(entity);
        auto rinstance = rm.getInstance(entity);
        auto linstance = lm.getInstance(entity);
        intptr_t treeNodeId = 1 + entity.getId();

        std::vector<utils::Entity> children(tm.getChildCount(tinstance));
        if (rinstance) {
            const char* entityName = mAsset->getName(entity);
            auto instance = rm.getInstance(entity);
            size_t numPrims = rm.getPrimitiveCount(instance);

            auto entityMaterial = mTweakedMaterials.find(entityName);
            if (entityMaterial != mTweakedMaterials.end()) {
                // These attributes are only present in the Shapr general material
                const auto& tweaks = entityMaterial->second;
                for (size_t prim = 0; prim < numPrims; ++prim) {
                    const auto& matInstance = rm.getMaterialInstanceAt(instance, prim);

                    auto setTextureIfPresent ([&](bool useTexture, const auto& filename, const std::string& propertyName) {
                        std::string samplerName = propertyName + "Texture";


                        auto textureEntry = mTextures.find(filename.asString());
                        if (useTexture && textureEntry != mTextures.end() && textureEntry->second != nullptr) {
                            matInstance->setParameter(samplerName.c_str(), textureEntry->second, trilinSampler);
                        }

                    });

                    std::uint32_t usageFlags = 0u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mBaseColor.isFile) << 0u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mClearCoatNormal.isFile) << 1u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mClearCoatRoughness.isFile) << 2u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mMetallic.isFile) << 3u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mNormal.isFile) << 4u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mOcclusion.isFile) << 5u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mRoughness.isFile) << 6u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mSheenRoughness.isFile) << 7u;
                    usageFlags |= 0u << 8u; // useSwizzledNormalMaps bit
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mTransmission.isFile) << 9u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mUseWard) << 10u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mAbsorption.useDerivedQuantity) << 11u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mSheenColor.useDerivedQuantity) << 12u;
                    usageFlags |= static_cast<std::uint32_t>(tweaks.mSubsurfaceColor.useDerivedQuantity) << 13u;
                    matInstance->setParameter("usageFlags", usageFlags);

                    setTextureIfPresent(tweaks.mBaseColor.isFile, tweaks.mBaseColor.filename, "baseColor");

                    matInstance->setParameter("tintColor", tweaks.mTintColor.value);
                    filament::math::float2 emissiveControl = {
                        tweaks.mEmissiveIntensity.value,
                        tweaks.mEmissiveExposureWeight.value
                    };
                    matInstance->setParameter("emissiveControl", emissiveControl);

                    filament::math::float4 basicIntensities = {
                        tweaks.mNormalIntensity.value,
                        tweaks.mClearCoatNormalIntensity.value,
                        tweaks.mSpecularIntensity.value,
                        tweaks.mOcclusionIntensity.value
                    };
                    matInstance->setParameter("basicIntensities", basicIntensities);

                    setTextureIfPresent(tweaks.mNormal.isFile, tweaks.mNormal.filename, "normal");
                    matInstance->setParameter("roughnessScale", tweaks.mRoughnessScale.value);
                    setTextureIfPresent(tweaks.mRoughness.isFile, tweaks.mRoughness.filename, "roughness");
                    matInstance->setParameter("roughnessUvScaler", tweaks.mRoughnessUvScaler.value);
                    if (tweaks.mShaderType == TweakableMaterial::MaterialType::Opaque || tweaks.mShaderType == TweakableMaterial::MaterialType::Cloth || tweaks.mShaderType == TweakableMaterial::MaterialType::Subsurface || tweaks.mShaderType == TweakableMaterial::MaterialType::Refractive) {
                        setTextureIfPresent(tweaks.mOcclusion.isFile, tweaks.mOcclusion.filename, "occlusion");
                        matInstance->setParameter("occlusion", tweaks.mOcclusion.value);
                    }

                    setTextureIfPresent(tweaks.mClearCoatNormal.isFile, tweaks.mClearCoatNormal.filename, "clearCoatNormal");
                    setTextureIfPresent(tweaks.mClearCoatRoughness.isFile, tweaks.mClearCoatRoughness.filename, "clearCoatRoughness");

                    matInstance->setParameter("textureScaler", math::float4(tweaks.mBaseTextureScale, tweaks.mNormalTextureScale, tweaks.mClearCoatTextureScale, tweaks.mRefractiveTextureScale));
                    math::float4 gammaBaseColor{}; 
                    gammaBaseColor.r = std::pow(tweaks.mBaseColor.value.r, 2.22f); 
                    gammaBaseColor.g = std::pow(tweaks.mBaseColor.value.g, 2.22f);
                    gammaBaseColor.b = std::pow(tweaks.mBaseColor.value.b, 2.22f);
                    gammaBaseColor.a = tweaks.mBaseColor.value.a;
                    matInstance->setParameter("baseColor", gammaBaseColor);
                    matInstance->setParameter("roughness", tweaks.mRoughness.value);
                    matInstance->setParameter("clearCoat", tweaks.mClearCoat.value);
                    matInstance->setParameter("clearCoatRoughness", tweaks.mClearCoatRoughness.value);
                    
                    if (tweaks.mShaderType != TweakableMaterial::MaterialType::Cloth) {
                        setTextureIfPresent(tweaks.mMetallic.isFile, tweaks.mMetallic.filename, "metallic");
                        matInstance->setParameter("metallic", tweaks.mMetallic.value);
                    }

                    if (tweaks.mShaderType == TweakableMaterial::MaterialType::Cloth) {
                        if (tweaks.mSheenColor.useDerivedQuantity) {
                            matInstance->setParameter("sheenIntensity", tweaks.mSheenIntensity.value);
                        } else {
                            matInstance->setParameter("sheenColor", tweaks.mSheenColor.value);
                            matInstance->setParameter("sheenIntensity", 1.0f);
                        }
                        matInstance->setParameter("subsurfaceColor", tweaks.mSubsurfaceColor.value);
                        matInstance->setParameter("subsurfaceTint", tweaks.mSubsurfaceTint.value * tweaks.mSubsurfaceIntensity.value);
                    } else if (tweaks.mShaderType == TweakableMaterial::MaterialType::Subsurface) {
                        matInstance->setParameter("thickness", tweaks.mThickness.value);
                        matInstance->setParameter("subsurfaceColor", tweaks.mSubsurfaceColor.value);
                        matInstance->setParameter("subsurfacePower", tweaks.mSubsurfacePower.value);
                        matInstance->setParameter("subsurfaceTint", tweaks.mSubsurfaceTint.value * tweaks.mSubsurfaceIntensity.value);
                    }

                    if (tweaks.mShaderType == TweakableMaterial::MaterialType::Opaque || tweaks.mShaderType == TweakableMaterial::MaterialType::Refractive) {
                        // Transparent materials do not expose anisotropy and sheen, these are not present in their UBOs
                        matInstance->setParameter("anisotropy", tweaks.mAnisotropy.value);
                        matInstance->setParameter("anisotropyDirection", normalize(tweaks.mAnisotropyDirection.value));

                        if (tweaks.mSheenColor.useDerivedQuantity) {
                            matInstance->setParameter("sheenIntensity", tweaks.mSheenIntensity.value);
                        }
                        else {
                            matInstance->setParameter("sheenColor", tweaks.mSheenColor.value);
                            matInstance->setParameter("sheenIntensity", 1.0f);
                        }
                        if (tweaks.mShaderType == TweakableMaterial::MaterialType::Opaque) {
                            setTextureIfPresent(tweaks.mSheenRoughness.isFile, tweaks.mSheenRoughness.filename, "sheenRoughness");
                        }
                        matInstance->setParameter("sheenRoughness", tweaks.mSheenRoughness.value);

                        if (tweaks.mShaderType == TweakableMaterial::MaterialType::Refractive) {
                            // Only refractive materials have the properties below
                            if (!tweaks.mAbsorption.useDerivedQuantity) {
                                matInstance->setParameter("absorption", tweaks.mAbsorption.value);
                            }

                            setTextureIfPresent(tweaks.mTransmission.isFile, tweaks.mTransmission.filename, "transmission");

                            matInstance->setParameter("iorScale", tweaks.mIorScale.value);
                            matInstance->setParameter("ior", tweaks.mIor.value);
                            matInstance->setParameter("transmission", tweaks.mTransmission.value);
                            matInstance->setParameter("thickness", tweaks.mThickness.value);
                            matInstance->setParameter("maxThickness", tweaks.mMaxThickness.value);
                        }
                    } 
                }
            }
        }
        tm.getChildren(tinstance, children.data(), children.size());
        for (auto ce : children) {
            applyMaterialSettingsInTree(ce);
        }
    };

    // Disable rounding and draw a fixed-height ImGui window that looks like a sidebar.
    ImGui::GetStyle().WindowRounding = 0;
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    ImGui::SetNextWindowSize(ImVec2(mSidebarWidth, height), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(20, height), ImVec2(width, height));

    ImGui::Begin("Filament", nullptr, ImGuiWindowFlags_NoTitleBar);
    if (mCustomUI) {
        mCustomUI();
    }

    DebugRegistry& debug = mEngine->getDebugRegistry();

    if (ImGui::CollapsingHeader("View")) {
        ImGui::Indent();

        ImGui::Checkbox("Post-processing", &mSettings.view.postProcessingEnabled);
        ImGui::Indent();
            bool dither = mSettings.view.dithering == Dithering::TEMPORAL;
            ImGui::Checkbox("Dithering", &dither);
            enableDithering(dither);
            ImGui::Checkbox("Bloom", &mSettings.view.bloom.enabled);
            ImGui::Checkbox("TAA", &mSettings.view.taa.enabled);
            // this clutters the UI and isn't that useful (except when working on TAA)
            //ImGui::Indent();
            //ImGui::SliderFloat("feedback", &mSettings.view.taa.feedback, 0.0f, 1.0f);
            //ImGui::SliderFloat("filter", &mSettings.view.taa.filterWidth, 0.0f, 2.0f);
            //ImGui::Unindent();

            bool fxaa = mSettings.view.antiAliasing == AntiAliasing::FXAA;
            ImGui::Checkbox("FXAA", &fxaa);
            enableFxaa(fxaa);
        ImGui::Unindent();

        ImGui::Checkbox("MSAA 4x", &mSettings.view.msaa.enabled);
        ImGui::Indent();
            ImGui::Checkbox("Custom resolve", &mSettings.view.msaa.customResolve);
        ImGui::Unindent();

        ImGui::Checkbox("SSAO", &mSettings.view.ssao.enabled);

        ImGui::Checkbox("Screen-space reflections", &mSettings.view.screenSpaceReflections.enabled);
        ImGui::Unindent();

        ImGui::Checkbox("Screen-space Guard Band", &mSettings.view.guardBand.enabled);
    }

    if (ImGui::CollapsingHeader("Bloom Options")) {
        ImGui::SliderFloat("Strength", &mSettings.view.bloom.strength, 0.0f, 1.0f);
        ImGui::Checkbox("Threshold", &mSettings.view.bloom.threshold);

        int levels = mSettings.view.bloom.levels;
        ImGui::SliderInt("Levels", &levels, 3, 11);
        mSettings.view.bloom.levels = levels;

        int quality = (int) mSettings.view.bloom.quality;
        ImGui::SliderInt("Bloom Quality", &quality, 0, 3);
        mSettings.view.bloom.quality = (View::QualityLevel) quality;

        ImGui::Checkbox("Lens Flare", &mSettings.view.bloom.lensFlare);
    }

    if (ImGui::CollapsingHeader("TAA Options")) {
        ImGui::Checkbox("Upscaling", &mSettings.view.taa.upscaling);
        ImGui::Checkbox("History Reprojection", &mSettings.view.taa.historyReprojection);
        ImGui::SliderFloat("Feedback", &mSettings.view.taa.feedback, 0.0f, 1.0f);
        ImGui::Checkbox("Filter History", &mSettings.view.taa.filterHistory);
        ImGui::Checkbox("Filter Input", &mSettings.view.taa.filterInput);
        ImGui::SliderFloat("FilterWidth", &mSettings.view.taa.filterWidth, 0.2f, 2.0f);
        ImGui::SliderFloat("LOD bias", &mSettings.view.taa.lodBias, -8.0f, 0.0f);
        ImGui::Checkbox("Use YCoCg", &mSettings.view.taa.useYCoCg);
        ImGui::Checkbox("Prevent Flickering", &mSettings.view.taa.preventFlickering);
        int jitterSequence = (int)mSettings.view.taa.jitterPattern;
        int boxClipping = (int)mSettings.view.taa.boxClipping;
        int boxType = (int)mSettings.view.taa.boxType;
        ImGui::Combo("Jitter Pattern", &jitterSequence, "RGSS x4\0Uniform Helix x4\0Halton x8\0Halton x16\0Halton x32\0\0");
        ImGui::Combo("Box Clipping", &boxClipping, "Accurate\0Clamp\0None\0\0");
        ImGui::Combo("Box Type", &boxType, "AABB\0Variance\0Both\0\0");
        ImGui::SliderFloat("Variance Gamma", &mSettings.view.taa.varianceGamma, 0.75f, 1.25f);
        ImGui::SliderFloat("RCAS", &mSettings.view.taa.sharpness, 0.0f, 1.0f);
        mSettings.view.taa.boxClipping = (TemporalAntiAliasingOptions::BoxClipping)boxClipping;
        mSettings.view.taa.boxType = (TemporalAntiAliasingOptions::BoxType)boxType;
        mSettings.view.taa.jitterPattern = (TemporalAntiAliasingOptions::JitterPattern)jitterSequence;
    }

    if (ImGui::CollapsingHeader("SSAO Options")) {
        auto& ssao = mSettings.view.ssao;

        int quality = (int) ssao.quality;
        int lowpass = (int) ssao.lowPassFilter;
        bool upsampling = ssao.upsampling != View::QualityLevel::LOW;

            ImGui::SliderFloat("SSAO power (contrast)", &ssao.power, 0.0f, 8.0f);
            ImGui::SliderFloat("SSAO intensity", &ssao.intensity, 0.0f, 2.0f);
            ImGui::SliderFloat("SSAO radius (meters)", &ssao.radius, 0.0f, 10.0f);
            ImGui::SliderFloat("SSAO bias (meters)", &ssao.bias, 0.0f, 0.1f, "%.6f");

            static int ssaoRes = 2;
            ImGui::RadioButton("SSAO half resolution", &ssaoRes, 1);
            ImGui::RadioButton("SSAO fullresolution", &ssaoRes, 2);
            ssao.resolution = ssaoRes * 0.5f;

        bool halfRes = ssao.resolution != 1.0f;
        ImGui::SliderInt("Quality", &quality, 0, 3);
        ImGui::SliderInt("Low Pass", &lowpass, 0, 2);
        ImGui::Checkbox("Bent Normals", &ssao.bentNormals);
        ImGui::Checkbox("High quality upsampling", &upsampling);
        ImGui::SliderFloat("Min Horizon angle", &ssao.minHorizonAngleRad, 0.0f, (float)M_PI_4);
        ImGui::SliderFloat("Bilateral Threshold", &ssao.bilateralThreshold, 0.0f, 0.1f);
        ImGui::Checkbox("Half resolution", &halfRes);
        ssao.resolution = halfRes ? 0.5f : 1.0f;

        ssao.upsampling = upsampling ? View::QualityLevel::HIGH : View::QualityLevel::LOW;
        ssao.lowPassFilter = (View::QualityLevel) lowpass;
        ssao.quality = (View::QualityLevel) quality;

        if (ImGui::CollapsingHeader("Dominant Light Shadows (experimental)")) {
            int sampleCount = ssao.ssct.sampleCount;
            ImGui::Checkbox("Enabled##dls", &ssao.ssct.enabled);
            ImGui::SliderFloat("Cone angle", &ssao.ssct.lightConeRad, 0.0f, (float)M_PI_2);
            ImGui::SliderFloat("Shadow Distance", &ssao.ssct.shadowDistance, 0.0f, 10.0f);
            ImGui::SliderFloat("Contact dist max", &ssao.ssct.contactDistanceMax, 0.0f, 100.0f);
            ImGui::SliderFloat("Intensity##dls", &ssao.ssct.intensity, 0.0f, 10.0f);
            ImGui::SliderFloat("Depth bias", &ssao.ssct.depthBias, 0.0f, 1.0f);
            ImGui::SliderFloat("Depth slope bias", &ssao.ssct.depthSlopeBias, 0.0f, 1.0f);
            ImGui::SliderInt("Sample Count", &sampleCount, 1, 32);
            ImGuiExt::DirectionWidget("Direction##dls", ssao.ssct.lightDirection.v);
            ssao.ssct.sampleCount = sampleCount;
        }
    }

    if (ImGui::CollapsingHeader("Screen-space reflections Options")) {
        auto& ssrefl = mSettings.view.screenSpaceReflections;
        ImGui::SliderFloat("Ray thickness", &ssrefl.thickness, 0.001f, 0.2f);
        ImGui::SliderFloat("Bias", &ssrefl.bias, 0.001f, 0.5f);
        ImGui::SliderFloat("Max distance", &ssrefl.maxDistance, 0.1, 10.0f);
        ImGui::SliderFloat("Stride", &ssrefl.stride, 1.0, 10.0f);
    }

    if (ImGui::CollapsingHeader("Dynamic Resolution")) {
        auto& dsr = mSettings.view.dsr;
        int quality = (int)dsr.quality;
        ImGui::Checkbox("enabled", &dsr.enabled);
        ImGui::Checkbox("homogeneous", &dsr.homogeneousScaling);
        ImGui::SliderFloat("min. scale", &dsr.minScale.x, 0.25f, 1.0f);
        ImGui::SliderFloat("max. scale", &dsr.maxScale.x, 0.25f, 1.0f);
        ImGui::SliderInt("quality", &quality, 0, 3);
        ImGui::SliderFloat("sharpness", &dsr.sharpness, 0.0f, 1.0f);
        dsr.minScale.x = std::min(dsr.minScale.x, dsr.maxScale.x);
        dsr.minScale.y = dsr.minScale.x;
        dsr.maxScale.y = dsr.maxScale.x;
        dsr.quality = (QualityLevel)quality;
    }

    auto& light = mSettings.lighting;
    auto& iblOptions = mSettings.lighting.iblOptions;
    if (ImGui::CollapsingHeader("Light")) {
        ImGui::Indent();
        if (ImGui::CollapsingHeader("Skybox")) {
            ImGui::SliderFloat("Intensity", &light.skyIntensity, 0.0f, 100000.0f);
        }

        if (ImGui::CollapsingHeader("Indirect light")) {
            ImGui::SliderFloat("IBL intensity", &light.iblIntensity, 0.0f, 100000.0f);
            ImGui::SliderAngle("IBL rotation", &light.iblRotation);

            ImGui::SliderFloat("IBL tint intensity", &iblOptions.iblTintAndIntensity.w, 0.0f, 1.0f);

            if (ImGui::RadioButton("Infinite", iblOptions.iblTechnique == IblOptions::IblTechnique::IBL_INFINITE)) {
                iblOptions.iblTechnique = IblOptions::IblTechnique::IBL_INFINITE;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Sphere", iblOptions.iblTechnique == IblOptions::IblTechnique::IBL_FINITE_SPHERE)) {
                iblOptions.iblTechnique = IblOptions::IblTechnique::IBL_FINITE_SPHERE;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Box", iblOptions.iblTechnique == IblOptions::IblTechnique::IBL_FINITE_BOX)) {
                iblOptions.iblTechnique = IblOptions::IblTechnique::IBL_FINITE_BOX;
            }

            if (iblOptions.iblTechnique == IblOptions::IblTechnique::IBL_FINITE_SPHERE) {
                static float radius = std::sqrt(iblOptions.iblHalfExtents.x);

                ImGui::SliderFloat3("Sphere center", iblOptions.iblCenter.v, -10.0f, 10.0f);
                ImGui::SliderFloat("Sphere radius", &radius, 0.0f, 256.0f);

                iblOptions.iblHalfExtents.x = radius * radius;
            }
            else if (iblOptions.iblTechnique == IblOptions::IblTechnique::IBL_FINITE_BOX) {
                static filament::math::float3 iblHalfExtents = iblOptions.iblHalfExtents;

                ImGui::SliderFloat3("Box center", iblOptions.iblCenter.v, -10.0f, 10.0f);
                ImGui::SliderFloat3("Box half extents", iblHalfExtents.v, -10.0f, 10.0f);

                iblOptions.iblHalfExtents = iblHalfExtents;
            }
        }
        if (ImGui::CollapsingHeader("Sunlight")) {
            ImGui::Checkbox("Enable sunlight", &light.enableSunlight);
            ImGui::SliderFloat("Sun intensity", &light.sunlightIntensity, 0.0f, 150000.0f);
            ImGui::SliderFloat("Halo size", &light.sunlightHaloSize, 1.01f, 40.0f);
            ImGui::SliderFloat("Halo falloff", &light.sunlightHaloFalloff, 4.0f, 1024.0f);
            ImGui::SliderFloat("Sun radius", &light.sunlightAngularRadius, 0.1f, 10.0f);
            ImGuiExt::DirectionWidget("Sun direction", light.sunlightDirection.v);
            ImGui::ColorEdit3("Sun color", light.sunlightColor.v);
            if (ImGui::Button("Reset Sunlight from IBL")) setSunlightFromIbl();

            ImGui::SliderFloat("Shadow Far", &light.shadowOptions.shadowFar, 0.0f,
                    mSettings.viewer.cameraFar);

            if (ImGui::CollapsingHeader("Shadow direction")) {
                float3 shadowDirection = light.shadowOptions.transform * light.sunlightDirection;
                ImGuiExt::DirectionWidget("Shadow direction", shadowDirection.v);
                light.shadowOptions.transform = normalize(quatf{
                        cross(light.sunlightDirection, shadowDirection),
                        sqrt(length2(light.sunlightDirection) * length2(shadowDirection))
                        + dot(light.sunlightDirection, shadowDirection)
                });
            }
        }
        if (ImGui::CollapsingHeader("Shadows")) {
            ImGui::Checkbox("Enable shadows", &light.enableShadows);
            int mapSize = light.shadowOptions.mapSize;
            ImGui::SliderInt("Shadow map size", &mapSize, 32, 4096);
            light.shadowOptions.mapSize = mapSize;
            ImGui::Checkbox("Stable Shadows", &light.shadowOptions.stable);
            ImGui::Checkbox("Enable LiSPSM", &light.shadowOptions.lispsm);

            int shadowType = (int)mSettings.view.shadowType;
            ImGui::Combo("Shadow type", &shadowType, "PCF\0VSM\0DPCF\0PCSS\0PCFd\0\0");
            mSettings.view.shadowType = (ShadowType)shadowType;

            if (mSettings.view.shadowType == ShadowType::VSM) {
                ImGui::Checkbox("High precision", &mSettings.view.vsmShadowOptions.highPrecision);
                ImGui::Checkbox("ELVSM", &mSettings.lighting.shadowOptions.vsm.elvsm);
                char label[32];
                snprintf(label, 32, "%d", 1 << mVsmMsaaSamplesLog2);
                ImGui::SliderInt("VSM MSAA samples", &mVsmMsaaSamplesLog2, 0, 3, label);
                mSettings.view.vsmShadowOptions.msaaSamples =
                        static_cast<uint8_t>(1u << mVsmMsaaSamplesLog2);

                int vsmAnisotropy = mSettings.view.vsmShadowOptions.anisotropy;
                snprintf(label, 32, "%d", 1 << vsmAnisotropy);
                ImGui::SliderInt("VSM anisotropy", &vsmAnisotropy, 0, 3, label);
                mSettings.view.vsmShadowOptions.anisotropy = vsmAnisotropy;
                ImGui::Checkbox("VSM mipmapping", &mSettings.view.vsmShadowOptions.mipmapping);
                ImGui::SliderFloat("VSM blur", &light.shadowOptions.vsm.blurWidth, 0.0f, 125.0f);

                // These are not very useful in practice (defaults are good), but we keep them here for debugging
                //ImGui::SliderFloat("VSM exponent", &mSettings.view.vsmShadowOptions.exponent, 0.0, 6.0f);
                //ImGui::SliderFloat("VSM Light bleed", &mSettings.view.vsmShadowOptions.lightBleedReduction, 0.0, 1.0f);
                //ImGui::SliderFloat("VSM min variance scale", &mSettings.view.vsmShadowOptions.minVarianceScale, 0.0, 10.0f);
            } else if (mSettings.view.shadowType == ShadowType::DPCF || mSettings.view.shadowType == ShadowType::PCSS) {
                ImGui::SliderFloat("Penumbra scale", &light.softShadowOptions.penumbraScale, 0.0f, 100.0f);
                ImGui::SliderFloat("Penumbra Ratio scale", &light.softShadowOptions.penumbraRatioScale, 1.0f, 100.0f);
            }

            int shadowCascades = light.shadowOptions.shadowCascades;
            ImGui::SliderInt("Cascades", &shadowCascades, 1, 4);
            ImGui::Checkbox("Debug cascades",
                    debug.getPropertyAddress<bool>("d.shadowmap.visualize_cascades"));
            ImGui::Checkbox("Enable contact shadows", &light.shadowOptions.screenSpaceContactShadows);
            ImGui::SliderFloat("Split pos 0", &light.shadowOptions.cascadeSplitPositions[0], 0.0f, 1.0f);
            ImGui::SliderFloat("Split pos 1", &light.shadowOptions.cascadeSplitPositions[1], 0.0f, 1.0f);
            ImGui::SliderFloat("Split pos 2", &light.shadowOptions.cascadeSplitPositions[2], 0.0f, 1.0f);
            light.shadowOptions.shadowCascades = shadowCascades;
        }
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Fog")) {
        int fogColorSource = 0;
        if (mSettings.view.fog.skyColor) {
            fogColorSource = 2;
        } else if (mSettings.view.fog.fogColorFromIbl) {
            fogColorSource = 1;
        }

        bool excludeSkybox = !std::isinf(mSettings.view.fog.cutOffDistance);
        ImGui::Indent();
        ImGui::Checkbox("Enable large-scale fog", &mSettings.view.fog.enabled);
        ImGui::SliderFloat("Start [m]", &mSettings.view.fog.distance, 0.0f, 100.0f);
        ImGui::SliderFloat("Extinction [1/m]", &mSettings.view.fog.density, 0.0f, 1.0f);
        ImGui::SliderFloat("Floor [m]", &mSettings.view.fog.height, 0.0f, 100.0f);
        ImGui::SliderFloat("Height falloff [1/m]", &mSettings.view.fog.heightFalloff, 0.0f, 4.0f);
        ImGui::SliderFloat("Sun Scattering start [m]", &mSettings.view.fog.inScatteringStart, 0.0f, 100.0f);
        ImGui::SliderFloat("Sun Scattering size", &mSettings.view.fog.inScatteringSize, 0.1f, 100.0f);
        ImGui::Checkbox("Exclude Skybox", &excludeSkybox);
        ImGui::Combo("Color##fogColor", &fogColorSource, "Constant\0IBL\0Skybox\0\0");
        ImGui::ColorPicker3("Color", mSettings.view.fog.color.v);
        ImGui::Unindent();
        mSettings.view.fog.cutOffDistance =
                excludeSkybox ? 1e6f : std::numeric_limits<float>::infinity();
        switch (fogColorSource) {
            case 0:
                mSettings.view.fog.skyColor = nullptr;
                mSettings.view.fog.fogColorFromIbl = false;
                break;
            case 1:
                mSettings.view.fog.skyColor = nullptr;
                mSettings.view.fog.fogColorFromIbl = true;
                break;
            case 2:
                mSettings.view.fog.skyColor = mSettings.view.fogSettings.fogColorTexture;
                mSettings.view.fog.fogColorFromIbl = false;
                break;
        }
    }

    if (ImGui::CollapsingHeader("Scene")) {
        ImGui::Indent();

        if (ImGui::Checkbox("Scale to unit cube", &mSettings.viewer.autoScaleEnabled)) {
            updateRootTransform();
        }

        ImGui::Checkbox("Automatic instancing", &mSettings.viewer.autoInstancingEnabled);

        int skyboxType = (int)mSettings.lighting.skyboxType;
        ImGui::Combo("Skybox type", &skyboxType,
            "Solid color\0Gradient\0Environment\0Checkerboard\0\0");
        mSettings.lighting.skyboxType = (decltype(mSettings.lighting.skyboxType))skyboxType;
        ImGui::ColorEdit3("Background color", &mSettings.viewer.backgroundColor.r);

        // We do not yet support ground shadow or scene selection in remote mode.
        if (!isRemoteMode()) {
            ImGui::Checkbox("Ground shadow", &mSettings.viewer.groundPlaneEnabled);
            ImGui::Indent();
            ImGui::SliderFloat("Strength", &mSettings.viewer.groundShadowStrength, 0.0f, 1.0f);
            ImGui::Unindent();

            if (mAsset->getSceneCount() > 1) {
                ImGui::Separator();
                sceneSelectionUI();
            }
        }

        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::Indent();

        ImGui::SliderFloat("Focal length (mm)", &mSettings.viewer.cameraFocalLength, 16.0f, 90.0f);
        ImGui::SliderFloat("Aperture", &mSettings.viewer.cameraAperture, 1.0f, 32.0f);
        ImGui::SliderFloat("Speed (1/s)", &mSettings.viewer.cameraSpeed, 1000.0f, 1.0f);
        ImGui::SliderFloat("ISO", &mSettings.viewer.cameraISO, 25.0f, 6400.0f);
        ImGui::SliderFloat("Near", &mSettings.viewer.cameraNear, 0.001f, 1.0f);
        ImGui::SliderFloat("Far", &mSettings.viewer.cameraFar, 1.0f, 10000.0f);

        if (ImGui::CollapsingHeader("DoF")) {
            bool dofMedian = mSettings.view.dof.filter == View::DepthOfFieldOptions::Filter::MEDIAN;
            int dofRingCount = mSettings.view.dof.fastGatherRingCount;
            int dofMaxCoC = mSettings.view.dof.maxForegroundCOC;
            if (!dofRingCount) dofRingCount = 5;
            if (!dofMaxCoC) dofMaxCoC = 32;
            ImGui::Checkbox("Enabled##dofEnabled", &mSettings.view.dof.enabled);
            ImGui::SliderFloat("Focus distance", &mSettings.viewer.cameraFocusDistance, 0.0f, 30.0f);
            ImGui::SliderFloat("Blur scale", &mSettings.view.dof.cocScale, 0.1f, 10.0f);
            ImGui::SliderFloat("CoC aspect-ratio", &mSettings.view.dof.cocAspectRatio, 0.25f, 4.0f);
            ImGui::SliderInt("Ring count", &dofRingCount, 1, 17);
            ImGui::SliderInt("Max CoC", &dofMaxCoC, 1, 32);
            ImGui::Checkbox("Native Resolution", &mSettings.view.dof.nativeResolution);
            ImGui::Checkbox("Median Filter", &dofMedian);
            mSettings.view.dof.filter = dofMedian ?
                                        View::DepthOfFieldOptions::Filter::MEDIAN :
                                        View::DepthOfFieldOptions::Filter::NONE;
            mSettings.view.dof.backgroundRingCount = dofRingCount;
            mSettings.view.dof.foregroundRingCount = dofRingCount;
            mSettings.view.dof.fastGatherRingCount = dofRingCount;
            mSettings.view.dof.maxForegroundCOC = dofMaxCoC;
            mSettings.view.dof.maxBackgroundCOC = dofMaxCoC;
        }

        if (ImGui::CollapsingHeader("Vignette")) {
            ImGui::Checkbox("Enabled##vignetteEnabled", &mSettings.view.vignette.enabled);
            ImGui::SliderFloat("Mid point", &mSettings.view.vignette.midPoint, 0.0f, 1.0f);
            ImGui::SliderFloat("Roundness", &mSettings.view.vignette.roundness, 0.0f, 1.0f);
            ImGui::SliderFloat("Feather", &mSettings.view.vignette.feather, 0.0f, 1.0f);
            ImGui::ColorEdit3("Color##vignetteColor", &mSettings.view.vignette.color.r);
        }

        // We do not yet support camera selection in the remote UI. To support this feature, we
        // would need to send a message from DebugServer to the WebSockets client.
        if (!isRemoteMode()) {

            const utils::Entity* cameras = mAsset->getCameraEntities();
            const size_t cameraCount = mAsset->getCameraEntityCount();

            std::vector<std::string> names;
            names.reserve(cameraCount + 1);
            names.emplace_back("Free camera");
            int c = 0;
            for (size_t i = 0; i < cameraCount; i++) {
                const char* n = mAsset->getName(cameras[i]);
                if (n) {
                    names.emplace_back(n);
                } else {
                    char buf[32];
                    sprintf(buf, "Unnamed camera %d", c++);
                    names.emplace_back(buf);
                }
            }

            std::vector<const char*> cstrings;
            cstrings.reserve(names.size());
            for (const auto & name : names) {
                cstrings.push_back(name.c_str());
            }

            ImGui::ListBox("Cameras", &mCurrentCamera, cstrings.data(), cstrings.size());
        }

        ImGui::Checkbox("Instanced stereo", &mSettings.view.stereoscopicOptions.enabled);
        ImGui::SliderFloat(
                "Ocular distance", &mSettings.viewer.cameraEyeOcularDistance, 0.0f, 1.0f);

        float toeInDegrees = mSettings.viewer.cameraEyeToeIn / f::PI * 180.0f;
        ImGui::SliderFloat("Toe in", &toeInDegrees, 0.0f, 30.0, "%.3f°");
        mSettings.viewer.cameraEyeToeIn = toeInDegrees / 180.0f * f::PI;

        ImGui::Unindent();
    }

    colorGradingUI(mSettings, mRangePlot, mCurvePlot, mToneMapPlot);

    // At this point, all View settings have been modified,
    //  so we can now push them into the Filament View.
    applySettings(mEngine, mSettings.view, mView);

    auto lights = utils::FixedCapacityVector<utils::Entity>::with_capacity(mScene->getEntityCount());
    mScene->forEach([&](utils::Entity entity) {
        if (lm.hasComponent(entity)) {
            lights.push_back(entity);
        }
    });

    applySettings(mEngine, mSettings.lighting, mIndirectLight, mSunlight,
            lights.data(), lights.size(), &lm, mScene, mView);

    // Set IBL options
    {        
        filament::math::float3 linearBgColor = filament::Color::toLinear(mSettings.viewer.backgroundColor);
        mSettings.lighting.iblOptions.iblTintAndIntensity.rgb = linearBgColor;
    }

    // TODO(prideout): add support for hierarchy, animation and variant selection in remote mode. To
    // support these features, we will need to send a message (list of strings) from DebugServer to
    // the WebSockets client.
    if (!isRemoteMode()) {
        if (ImGui::CollapsingHeader("Hierarchy")) {
            if (ImGui::Button("Make all visible")) {
                changeAllVisibility(mAsset->getRoot(), true);
                for (bool& isVisible : mVisibility) isVisible = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Make all invisible")) {
                changeAllVisibility(mAsset->getRoot(), false);
                for (bool& isVisible : mVisibility) isVisible = false;
            }
            ImGui::Indent();
            ImGui::Checkbox("Show bounds", &mEnableWireframe);
            treeNode(mAsset->getRoot());
            ImGui::Unindent();
        }

        applyMaterialSettingsInTree(mAsset->getRoot());

        if (mInstance->getMaterialVariantCount() > 0 && ImGui::CollapsingHeader("Variants")) {
            ImGui::Indent();
            int selectedVariant = mCurrentVariant;
            for (size_t i = 0, count = mInstance->getMaterialVariantCount(); i < count; ++i) {
                const char* label = mInstance->getMaterialVariantName(i);
                ImGui::RadioButton(label, &selectedVariant, i);
            }
            if (selectedVariant != mCurrentVariant) {
                mCurrentVariant = selectedVariant;
                mInstance->applyMaterialVariant(mCurrentVariant);
            }
            ImGui::Unindent();
        }

        Animator& animator = *mInstance->getAnimator();
        const size_t animationCount = animator.getAnimationCount();
        if (animationCount > 0 && ImGui::CollapsingHeader("Animation")) {
            ImGui::Indent();
            int selectedAnimation = mCurrentAnimation;
            ImGui::RadioButton("Disable", &selectedAnimation, -1);
            ImGui::RadioButton("Apply all animations", &selectedAnimation, animationCount);
            ImGui::SliderFloat("Cross fade", &mCrossFadeDuration, 0.0f, 2.0f,
                    "%4.2f seconds", ImGuiSliderFlags_AlwaysClamp);
            for (size_t i = 0; i < animationCount; ++i) {
                std::string label = animator.getAnimationName(i);
                if (label.empty()) {
                    label = "Unnamed " + std::to_string(i);
                }
                ImGui::RadioButton(label.c_str(), &selectedAnimation, i);
            }
            if (selectedAnimation != mCurrentAnimation) {
                mPreviousAnimation = mCurrentAnimation;
                mCurrentAnimation = selectedAnimation;
                mResetAnimation = true;
            }
            ImGui::Checkbox("Show rest pose", &mShowingRestPose);
            ImGui::Unindent();
        }

        if (mCurrentMorphingEntity && ImGui::CollapsingHeader("Morphing")) {
            const bool isAnimating = mCurrentAnimation > 0 && animator.getAnimationCount() > 0;
            if (isAnimating) {
                ImGui::BeginDisabled();
            }
            for (int i = 0; i != mMorphWeights.size(); ++i) {
                const char* name = mAsset->getMorphTargetNameAt(mCurrentMorphingEntity, i);
                std::string label = name ? name : "Unnamed target " + std::to_string(i);
                ImGui::SliderFloat(label.c_str(), &mMorphWeights[i], 0.0f, 1.0);
            }
            if (isAnimating) {
                ImGui::EndDisabled();
            }
            if (!isAnimating) {
                auto instance = rm.getInstance(mCurrentMorphingEntity);
                rm.setMorphWeights(instance, mMorphWeights.data(), mMorphWeights.size());
            }
        }

        if (mEnableWireframe) {
            mScene->addEntity(mAsset->getWireframe());
        } else {
            mScene->remove(mAsset->getWireframe());
        }
    }

    {
        // Camera movement speed setting UI
        ImGui::Separator();
        if (ImGui::SliderFloat("Cam move speed", &mSettings.viewer.cameraMovementSpeed, 1.0f, 100.0f)) {
            updateCameraMovementSpeed();
        }
    }

    {
        // Art root directory display
        ImGui::Separator();
        ImGui::Text("Art root: %s", mSettings.viewer.artRootPath.empty() ? "<none>" : mSettings.viewer.artRootPath.c_str());
        char tempArtRootPath[1024];
        if (ImGui::Button("Set art root") && mDoSaveSettings && SD_OpenFolderDialog(&tempArtRootPath[0])) {
            utils::Path tempPath = tempArtRootPath;
            if (tempPath.exists()) {
                auto sanitizePath = [](std::string& path) {
                    for (int i = 0; i < path.length(); ++i) {
                        if (path[i] == '\\') path[i] = '/';
                    }
                };

                mSettings.viewer.artRootPath = tempPath.c_str();
                sanitizePath(mSettings.viewer.artRootPath);
                g_ArtRootPathStr = tempArtRootPath;
                mDoSaveSettings();
            }
            else {
                // error
            }
        }
    }

    {
        // Hotkey-related feedback UI
        ImGui::Separator();
        ImGui::Text("Entity material to save: %s", mLastSavedEntityName.empty() ? "<none>" : mLastSavedEntityName.c_str());
        ImGui::Text("Save filename: %s", mLastSavedFileName.empty() ? "<none>" : mLastSavedFileName.c_str());
    }
    mSidebarWidth = ImGui::GetWindowWidth();
    ImGui::End();
}

// This loads the last saved (or loaded) material from file, to its entity
void ViewerGui::quickLoad() {
    loadTweaksFromFile(mLastSavedEntityName, mLastSavedFileName);
}

void ViewerGui::undoLastModification() {
    // TODO implement
}

void ViewerGui::updateCameraMovementSpeed() {
    if (mCameraMovementSpeedUpdateCallback) {
        mCameraMovementSpeedUpdateCallback(mSettings.viewer.cameraMovementSpeed);
    } else {
        std::cout << "Oops! No mCameraMovementSpeedUpdateCallback set!" << std::endl;
    }
}

void ViewerGui::setCameraMovementSpeedUpdateCallback(std::function<void(float)>&& callback) {
    mCameraMovementSpeedUpdateCallback = std::move(callback);
}

void ViewerGui::updateCameraSpeedOnUI(float value) {
    mSettings.viewer.cameraMovementSpeed = value;
}

} // namespace viewer
} // namespace filament
