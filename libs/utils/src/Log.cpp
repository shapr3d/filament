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

#include <utils/Log.h>

#include <string>
#include <utils/compiler.h>

#ifdef ANDROID
#   include <android/log.h>
#   ifndef UTILS_LOG_TAG
#       define UTILS_LOG_TAG "Filament"
#   endif
#endif

namespace utils {

namespace io {

class LogStream : public ostream {
public:

    enum Priority {
        LOG_DEBUG, LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_VERBOSE
    };

    explicit LogStream(Priority p) noexcept : mPriority(p) {}

    ostream& flush() noexcept override;

private:
    Priority mPriority;
};

ostream& LogStream::flush() noexcept {
    Buffer& buf = getBuffer();
#if ANDROID
    switch (mPriority) {
        case LOG_DEBUG:
            __android_log_write(ANDROID_LOG_DEBUG, UTILS_LOG_TAG, buf.get());
            break;
        case LOG_ERROR:
            __android_log_write(ANDROID_LOG_ERROR, UTILS_LOG_TAG, buf.get());
            break;
        case LOG_WARNING:
            __android_log_write(ANDROID_LOG_WARN, UTILS_LOG_TAG, buf.get());
            break;
        case LOG_INFO:
            __android_log_write(ANDROID_LOG_INFO, UTILS_LOG_TAG, buf.get());
            break;
        case LOG_VERBOSE:
            __android_log_write(ANDROID_LOG_VERBOSE, UTILS_LOG_TAG, buf.get());
            break;
    }
#else // ANDROID

    LoggerCallback callback = nullptr;
    FILE* stream = nullptr;

    switch (mPriority) {
        case LOG_DEBUG:
            callback = slogdcb;
            stream = stdout;
            break;
        case LOG_WARNING:
            callback = slogwcb;
            stream = stdout;
            break;
        case LOG_INFO:
            callback = slogicb;
            stream = stdout;
            break;
        case LOG_ERROR:
            callback = slogecb;
            stream = stderr;
            break;
        case LOG_VERBOSE:
#ifndef NDEBUG
            callback = slogvcb;
            stream = stdout;
#endif
            break;
    }
#endif // ANDROID

    if (callback) {
        callback(buf.get());
    } else if (stream) {
        fprintf(stream, "%s", buf.get());
    }

    buf.reset();
    return *this;
}

static LogStream cout(LogStream::Priority::LOG_DEBUG);
static LogStream cerr(LogStream::Priority::LOG_ERROR);
static LogStream cwarn(LogStream::Priority::LOG_WARNING);
static LogStream cinfo(LogStream::Priority::LOG_INFO);
static LogStream cverbose(LogStream::Priority::LOG_VERBOSE);

} // namespace io


Loggers const slog = {
        io::cout,       // debug
        io::cerr,       // error
        io::cwarn,      // warning
        io::cinfo,      // info
        io::cverbose    // verbose
};

LoggerCallback slogdcb = nullptr;
LoggerCallback slogecb = nullptr;
LoggerCallback slogwcb = nullptr;
LoggerCallback slogicb = nullptr;
LoggerCallback slogvcb = nullptr;

} // namespace utils
