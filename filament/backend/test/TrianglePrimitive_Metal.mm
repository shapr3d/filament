/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "TrianglePrimitive.h"

#import <Metal/Metal.h>

namespace test {

intptr_t TrianglePrimitive::createNativeVertexBuffer(size_t size) const noexcept {
    MTLResourceOptions options = MTLStorageModePrivate << MTLResourceStorageModeShift;
    id<MTLBuffer> buffer = [MTLCreateSystemDefaultDevice() newBufferWithLength:size options:options];
    if (!buffer) {
        return 0;
    }

    // We must retain the buffer before leaving the scope of this function, otherwise the id<> dtor would
    // free the buffer. A corresponding destroyNativeVertexBuffer() will release it
    intptr_t bridgedPtr = intptr_t((__bridge void*)buffer);
    CFRetain((CFTypeRef)bridgedPtr);
    return bridgedPtr;
}

void TrianglePrimitive::destroyNativeVertexBuffer(intptr_t nativeBuffer) const noexcept {
    CFRelease((CFTypeRef)nativeBuffer);
}

} // namespae test
