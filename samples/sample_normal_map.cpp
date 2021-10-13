/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <iostream>
#include <string>
#include <map>
#include <vector>

#include <getopt/getopt.h>

#include <utils/Path.h>

#include <filament/Engine.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/TextureSampler.h>
#include <filament/TransformManager.h>
#include <filament/Scene.h>
#include <filament/Texture.h>

#include <math/mat3.h>
#include <math/mat4.h>
#include <math/vec4.h>

#include <filamentapp/Config.h>
#include <filamentapp/FilamentApp.h>
#include <stb_image.h>

#include <utils/EntityManager.h>

#include <filamat/MaterialBuilder.h>
#include <filameshio/MeshReader.h>

using namespace filament::math;
using namespace filament;
using namespace filamesh;
using namespace filamat;
using namespace utils;

static std::vector<Path> g_filenames;

static MeshReader::MaterialRegistry g_materialInstances;
static std::vector<MeshReader::Mesh> g_meshes;
static const Material* g_material;
static Entity g_light;
static Texture* g_normalMap = nullptr;
static Texture* g_clearCoatNormalMap = nullptr;
static Texture* g_baseColorMap = nullptr;

static Config g_config;
static struct NormalConfig {
    std::string normalMap;
    std::string clearCoatNormalMap;
    std::string baseColorMap;
} g_normalConfig;

static void printUsage(char* name) {
    std::string exec_name(Path(name).getName());
    std::string usage(
            "SAMPLE_NORMAL_MAP is an example of normal mapping\n"
            "Usage:\n"
            "    SAMPLE_NORMAL_MAP [options] <filamesh input files>\n"
            "Options:\n"
            "   --help, -h\n"
            "       Prints this message\n\n"
            "   --api, -a\n"
            "       Specify the backend API: opengl (default), vulkan, or metal\n\n"
            "   --ibl=<path to cmgen IBL>, -i <path>\n"
            "       Applies an IBL generated by cmgen's deploy option\n\n"
            "   --split-view, -v\n"
            "       Splits the window into 4 views\n\n"
            "   --scale=[number], -s [number]\n"
            "       Applies uniform scale\n\n"
            "   --normal-map=<path to PNG/JPG/BMP/GIF/TGA/PSD>, -n <path>\n"
            "       Normal map to apply to the loaded meshes\n\n"
            "   --clear-coatnormal-map=<path to PNG/JPG/BMP/GIF/TGA/PSD>, -c <path>\n"
            "       Normal map to apply to the clear coat layer\n\n"
            "   --basecolor-map=<path to PNG/JPG/BMP/GIF/TGA/PSD>, -b <path>\n"
            "       Base color map to apply to the loaded meshes\n\n"
    );
    const std::string from("SAMPLE_NORMAL_MAP");
    for (size_t pos = usage.find(from); pos != std::string::npos; pos = usage.find(from, pos)) {
        usage.replace(pos, from.length(), exec_name);
    }
    std::cout << usage;
}

static int handleCommandLineArgments(int argc, char* argv[], Config* config) {
    static constexpr const char* OPTSTR = "ha:i:vs:n:a:c:b:";
    static const struct option OPTIONS[] = {
            { "help",                   no_argument,       nullptr, 'h' },
            { "api",                    required_argument, nullptr, 'a' },
            { "ibl",                    required_argument, nullptr, 'i' },
            { "split-view",             no_argument,       nullptr, 'v' },
            { "scale",                  required_argument, nullptr, 's' },
            { "normal-map",             required_argument, nullptr, 'n' },
            { "clear-coat-normal-map",  required_argument, nullptr, 'c' },
            { "basecolor-map",          required_argument, nullptr, 'b' },
            { nullptr, 0, nullptr, 0 }  // termination of the option list
    };
    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, OPTSTR, OPTIONS, &option_index)) >= 0) {
        std::string arg(optarg ? optarg : "");
        switch (opt) {
            default:
            case 'h':
                printUsage(argv[0]);
                exit(0);
            case 'a':
                if (arg == "opengl") {
                    config->backend = Engine::Backend::OPENGL;
                } else if (arg == "vulkan") {
                    config->backend = Engine::Backend::VULKAN;
                } else if (arg == "metal") {
                    config->backend = Engine::Backend::METAL;
                } else {
                    std::cerr << "Unrecognized backend. Must be 'opengl'|'vulkan'|'metal'." << std::endl;
                }
                break;
            case 'i':
                config->iblDirectory = arg;
                break;
            case 's':
                try {
                    config->scale = std::stof(arg);
                } catch (std::invalid_argument& e) {
                    // keep scale of 1.0
                } catch (std::out_of_range& e) {
                    // keep scale of 1.0
                }
                break;
            case 'v':
                config->splitView = true;
                break;
            case 'n':
                g_normalConfig.normalMap = arg;
                break;
            case 'c':
                g_normalConfig.clearCoatNormalMap = arg;
                break;
            case 'b':
                g_normalConfig.baseColorMap = arg;
                break;
        }
    }

    return optind;
}

static void cleanup(Engine* engine, View*, Scene*) {
    engine->destroy(g_normalMap);
    engine->destroy(g_clearCoatNormalMap);
    std::vector<filament::MaterialInstance*> materialList(g_materialInstances.numRegistered());
    g_materialInstances.getRegisteredMaterials(materialList.data());
    for (auto material : materialList) {
        engine->destroy(material);
    }
    g_materialInstances.unregisterAll();
    engine->destroy(g_material);
    EntityManager& em = EntityManager::get();
    for (auto mesh : g_meshes) {
        engine->destroy(mesh.vertexBuffer);
        engine->destroy(mesh.indexBuffer);
        engine->destroy(mesh.renderable);
        em.destroy(mesh.renderable);
    }
    engine->destroy(g_light);
    em.destroy(g_light);
}

void loadNormalMap(Engine* engine, Texture** normalMap, const std::string& path) {
    if (!path.empty()) {
        Path p(path);
        if (p.exists()) {
            int w, h, n;
            unsigned char* data = stbi_load(p.getAbsolutePath().c_str(), &w, &h, &n, 3);
            if (data != nullptr) {
                *normalMap = Texture::Builder()
                        .width(uint32_t(w))
                        .height(uint32_t(h))
                        .levels(0xff)
                        .format(Texture::InternalFormat::RGB8)
                        .build(*engine);
                Texture::PixelBufferDescriptor buffer(data, size_t(w * h * 3),
                        Texture::Format::RGB, Texture::Type::UBYTE,
                        (Texture::PixelBufferDescriptor::Callback)&stbi_image_free);
                (*normalMap)->setImage(*engine, 0, std::move(buffer));
                (*normalMap)->generateMipmaps(*engine);
            } else {
                std::cout << "The normal map " << p << " could not be loaded" << std::endl;
            }
        } else {
            std::cout << "The normal map " << p << " does not exist" << std::endl;
        }
    }
}

void loadBaseColorMap(Engine* engine) {
    if (!g_normalConfig.baseColorMap.empty()) {
        Path path(g_normalConfig.baseColorMap);
        if (path.exists()) {
            int w, h, n;
            unsigned char* data = stbi_load(path.getAbsolutePath().c_str(), &w, &h, &n, 3);
            if (data != nullptr) {
                g_baseColorMap = Texture::Builder()
                        .width(uint32_t(w))
                        .height(uint32_t(h))
                        .levels(0xff)
                        .format(Texture::InternalFormat::SRGB8)
                        .build(*engine);
                Texture::PixelBufferDescriptor buffer(data, size_t(w * h * 3),
                        Texture::Format::RGB, Texture::Type::UBYTE,
                        (Texture::PixelBufferDescriptor::Callback)&stbi_image_free);
                g_baseColorMap->setImage(*engine, 0, std::move(buffer));
                g_baseColorMap->generateMipmaps(*engine);
            } else {
                std::cout << "The base color map " << path << " could not be loaded" << std::endl;
            }
        } else {
            std::cout << "The base color map " << path << " does not exist" << std::endl;
        }
    }
}

static void setup(Engine* engine, View*, Scene* scene) {
    loadNormalMap(engine, &g_normalMap, g_normalConfig.normalMap);
    loadNormalMap(engine, &g_clearCoatNormalMap, g_normalConfig.clearCoatNormalMap);
    loadBaseColorMap(engine);

    bool hasNormalMap = g_normalMap != nullptr;
    bool hasClearCoatNormalMap = g_clearCoatNormalMap != nullptr;
    bool hasBaseColorMap = g_baseColorMap != nullptr;

    std::string shader = R"SHADER(
        void material(inout MaterialInputs material) {
    )SHADER";

    if (hasNormalMap) {
        shader += R"SHADER(
            material.normal = texture(materialParams_normalMap, getUV0()).xyz * 2.0 - 1.0;
        )SHADER";
    }

    if (hasClearCoatNormalMap) {
        shader += R"SHADER(
            material.clearCoatNormal =
                    texture(materialParams_clearCoatNormalMap, getUV0()).xyz * 2.0 - 1.0;
        )SHADER";
    }

    shader += R"SHADER(
        prepareMaterial(material);
    )SHADER";

    if (hasBaseColorMap) {
        shader += R"SHADER(
            material.baseColor.rgb = texture(materialParams_baseColorMap, getUV0()).rgb;
            material.metallic = 1.0;
            material.roughness = 0.6;
        )SHADER";
    } else {
        shader += R"SHADER(
            material.baseColor.rgb = float3(0.3, 0.0, 0.0);
            material.metallic = 0.0;
            material.roughness = 0.0;
        )SHADER";
    }

    if (hasClearCoatNormalMap) {
        shader += "    material.clearCoat = 1.0;\n";
    }

    shader += "}\n";

    MaterialBuilder::init();
    MaterialBuilder builder = MaterialBuilder()
            .name("DefaultMaterial")
            .targetApi(MaterialBuilder::TargetApi::ALL)
#ifndef NDEBUG
            .optimization(MaterialBuilderBase::Optimization::NONE)
#endif
            .material(shader.c_str())
            .specularAntiAliasing(true)
            .shading(Shading::LIT);

    if (hasNormalMap) {
        builder
            .require(VertexAttribute::UV0)
            .parameter(MaterialBuilder::SamplerType::SAMPLER_2D, "normalMap");
    }

    if (hasClearCoatNormalMap) {
        builder
            .require(VertexAttribute::UV0)
            .parameter(MaterialBuilder::SamplerType::SAMPLER_2D, "clearCoatNormalMap");
    }

    if (hasBaseColorMap) {
        builder
            .require(VertexAttribute::UV0)
            .parameter(MaterialBuilder::SamplerType::SAMPLER_2D, "baseColorMap");
    }

    Package pkg = builder.build(engine->getJobSystem());

    g_material = Material::Builder().package(pkg.getData(), pkg.getSize())
            .build(*engine);
    const utils::CString defaultMaterialName("DefaultMaterial");
    g_materialInstances.registerMaterialInstance(defaultMaterialName, g_material->createInstance());

    TextureSampler sampler(TextureSampler::MinFilter::LINEAR_MIPMAP_LINEAR,
            TextureSampler::MagFilter::LINEAR, TextureSampler::WrapMode::REPEAT);
    sampler.setAnisotropy(8.0f);

    if (hasNormalMap) {
        g_materialInstances.getMaterialInstance(defaultMaterialName)->setParameter("normalMap", g_normalMap, sampler);
    }
    if (hasClearCoatNormalMap) {
        g_materialInstances.getMaterialInstance(defaultMaterialName)->setParameter(
                "clearCoatNormalMap", g_clearCoatNormalMap, sampler);
    }
    if (hasBaseColorMap) {
        g_materialInstances.getMaterialInstance(defaultMaterialName)->setParameter(
                "baseColorMap", g_baseColorMap, sampler);
    }

    auto& tcm = engine->getTransformManager();
    for (const auto& filename : g_filenames) {
        MeshReader::Mesh mesh  = MeshReader::loadMeshFromFile(engine, filename, g_materialInstances);
        if (mesh.renderable) {
            auto ei = tcm.getInstance(mesh.renderable);
            tcm.setTransform(ei, mat4f{ mat3f(g_config.scale), float3(0.0f, 0.0f, -4.0f) } *
                                 tcm.getWorldTransform(ei));
            scene->addEntity(mesh.renderable);
            g_meshes.push_back(mesh);
        }
    }

    g_light = EntityManager::get().create();
    LightManager::Builder(LightManager::Type::DIRECTIONAL)
            .color(Color::toLinear<ACCURATE>({0.98f, 0.92f, 0.89f}))
            .intensity(110000)
            .direction({0.6, -1, -0.8})
            .build(*engine, g_light);
    scene->addEntity(g_light);
}

int main(int argc, char* argv[]) {
    int option_index = handleCommandLineArgments(argc, argv, &g_config);
    int num_args = argc - option_index;
    if (num_args < 1) {
        printUsage(argv[0]);
        return 1;
    }

    for (int i = option_index; i < argc; i++) {
        utils::Path filename = argv[i];
        if (!filename.exists()) {
            std::cerr << "file " << argv[i] << " not found!" << std::endl;
            return 1;
        }
        g_filenames.push_back(filename);
    }

    g_config.title = "Normal Mapping";
    FilamentApp& filamentApp = FilamentApp::get();
    filamentApp.run(g_config, setup, cleanup);

    return 0;
}
