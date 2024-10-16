/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef TNT_FILAMENT_DETAILS_INDEXBUFFER_H
#define TNT_FILAMENT_DETAILS_INDEXBUFFER_H

#include "downcast.h"

#include <backend/Handle.h>

#include <filament/IndexBuffer.h>

#include <utils/compiler.h>

namespace filament {

class FBufferObject;
class FEngine;

class FIndexBuffer : public IndexBuffer {
public:
    using IndexBufferHandle = backend::IndexBufferHandle;
    using BufferObjectHandle = backend::BufferObjectHandle;

    FIndexBuffer(FEngine& engine, const Builder& builder);

    // frees driver resources, object becomes invalid
    void terminate(FEngine& engine);

    IndexBufferHandle getHwHandle() const noexcept { return mHandle; }

    size_t getIndexCount() const noexcept { return mIndexCount; }

    void setBuffer(FEngine& engine, BufferDescriptor&& buffer, uint32_t byteOffset = 0);
    void setBufferObject(FEngine& engine, FBufferObject const* bufferObject);

private:
    friend class IndexBuffer;
    IndexBufferHandle mHandle;
    BufferObjectHandle mObjectHandle;
    uint32_t mIndexCount;
    bool mBufferObjectEnabled = false;
};

FILAMENT_DOWNCAST(IndexBuffer)

} // namespace filament

#endif // TNT_FILAMENT_DETAILS_INDEXBUFFER_H
