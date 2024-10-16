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

#ifndef TNT_UTILS_LOG_H
#define TNT_UTILS_LOG_H

#include <utils/compiler.h>
#include <utils/ostream.h>

namespace utils {

struct UTILS_PUBLIC Loggers {
    // DEBUG level logging stream
    io::ostream& d;

    // ERROR level logging stream
    io::ostream& e;

    // WARNING level logging stream
    io::ostream& w;

    // INFORMATION level logging stream
    io::ostream& i;

    // VERBOSE level logging stream
    io::ostream& v;
};

extern UTILS_PUBLIC Loggers const slog;

using LoggerCallback = void(*)(const char*);
extern UTILS_PUBLIC LoggerCallback slogdcb;
extern UTILS_PUBLIC LoggerCallback slogecb;
extern UTILS_PUBLIC LoggerCallback slogwcb;
extern UTILS_PUBLIC LoggerCallback slogicb;
extern UTILS_PUBLIC LoggerCallback slogvcb;

} // namespace utils

#endif // TNT_UTILS_LOG_H
