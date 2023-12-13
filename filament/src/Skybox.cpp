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

#include "details/Skybox.h"

#include "details/Texture.h"

namespace filament {

void Skybox::setLayerMask(uint8_t select, uint8_t values) noexcept {
    downcast(this)->setLayerMask(select, values);
}

uint8_t Skybox::getLayerMask() const noexcept {
    return downcast(this)->getLayerMask();
}

float Skybox::getIntensity() const noexcept {
    return downcast(this)->getIntensity();
}

void Skybox::setIntensity(float intensity) noexcept {
    downcast(this)->setIntensity(intensity);
}

void Skybox::setColor(math::float4 color) noexcept {
    downcast(this)->setColor(color);
}

void Skybox::setType(SkyboxType type) noexcept {
    downcast(this)->setType(type);
}

void Skybox::setUiScale(float scale) noexcept {
    downcast(this)->setUiScale(scale);
}

void Skybox::setUpDirectionAxis(UpDirectionAxis axis) noexcept {
    downcast(this)->setUpDirectionAxis(axis);
}

void Skybox::setCheckerboardGrays(math::float2 grays) noexcept {
    downcast(this)->setCheckerboardGrays(grays);
}

void Skybox::setPriority(uint8_t priority) noexcept {
    downcast(this)->setPriority(priority);
}

Texture const* Skybox::getTexture() const noexcept {
    return downcast(this)->getTexture();
}

} // namespace filament
