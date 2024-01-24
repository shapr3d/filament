// This file has been generated by beamsplitter

#include "Settings_generated.h"

#include <filament/Options.h>
#include <utils/Log.h>

#include <ostream>

#include "jsonParseUtils.h"

using namespace utils;

namespace filament::viewer {

// Compares a JSON string token against a C string.
int compare(jsmntok_t tok, const char* jsonChunk, const char* str) {
    size_t slen = strlen(str);
    size_t tlen = tok.end - tok.start;
    return (slen == tlen) ? strncmp(jsonChunk + tok.start, str, slen) : 128;
}

std::ostream& writeJson(std::ostream& oss, const float* v, int count) {
    oss << "[";
    for (int i = 0; i < count; i++) {
        oss << v[i];
        if (i < count - 1) {
            oss << ", ";
        }
    }
    oss << "]";
    return oss;
}

std::ostream& operator<<(std::ostream& out, math::float2 v) {
    return writeJson(out, v.v, 2);
}

std::ostream& operator<<(std::ostream& out, math::float3 v) {
    return writeJson(out, v.v, 3);
}

std::ostream& operator<<(std::ostream& out, math::float4 v) {
    return writeJson(out, v.v, 4);
}

const char* to_string(bool b) { return b ? "true" : "false"; }

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, uint8_t* val) {
    CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
    *val = strtol(jsonChunk + tokens[i].start, nullptr, 10);
    return i + 1;
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, uint16_t* val) {
    CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
    *val = strtol(jsonChunk + tokens[i].start, nullptr, 10);
    return i + 1;
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, uint32_t* val) {
    CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
    *val = strtol(jsonChunk + tokens[i].start, nullptr, 10);
    return i + 1;
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, int* val) {
    CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
    *val = strtol(jsonChunk + tokens[i].start, nullptr, 10);
    return i + 1;
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, float* val) {
    CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
    *val = strtod(jsonChunk + tokens[i].start, nullptr);
    return i + 1;
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, float* vals, int size) {
    CHECK_TOKTYPE(tokens[i], JSMN_ARRAY);
    if (tokens[i].size != size) {
        slog.w << "Expected " << size << " floats, got " << tokens[i].size << io::endl;
        return i + 1 + tokens[i].size;
    }
    ++i;
    for (int j = 0; j < size; ++j) {
        i = parse(tokens, i, jsonChunk, &vals[j]);
    }
    return i;
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, bool* val) {
    CHECK_TOKTYPE(tokens[i], JSMN_PRIMITIVE);
    if (0 == compare(tokens[i], jsonChunk, "true")) {
        *val = true;
        return i + 1;
    }
    if (0 == compare(tokens[i], jsonChunk, "false")) {
        *val = false;
        return i + 1;
    }
    return -1;
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, math::float2* val) {
    float values[2];
    i = parse(tokens, i, jsonChunk, values, 2);
    *val = {values[0], values[1]};
    return i;
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, math::float3* val) {
    float values[3];
    i = parse(tokens, i, jsonChunk, values, 3);
    *val = {values[0], values[1], values[2]};
    return i;
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, math::float4* val) {
    float values[4];
    i = parse(tokens, i, jsonChunk, values, 4);
    *val = {values[0], values[1], values[2], values[3]};
    return i;
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, QualityLevel* out) {
    if (0 == compare(tokens[i], jsonChunk, "LOW")) { *out = QualityLevel::LOW; }
    else if (0 == compare(tokens[i], jsonChunk, "MEDIUM")) { *out = QualityLevel::MEDIUM; }
    else if (0 == compare(tokens[i], jsonChunk, "HIGH")) { *out = QualityLevel::HIGH; }
    else if (0 == compare(tokens[i], jsonChunk, "ULTRA")) { *out = QualityLevel::ULTRA; }
    else {
        slog.w << "Invalid QualityLevel: '" << STR(tokens[i], jsonChunk) << "'" << io::endl;
    }
    return i + 1;
}

std::ostream& operator<<(std::ostream& out, QualityLevel in) {
    switch (in) {
        case QualityLevel::LOW: return out << "\"LOW\"";
        case QualityLevel::MEDIUM: return out << "\"MEDIUM\"";
        case QualityLevel::HIGH: return out << "\"HIGH\"";
        case QualityLevel::ULTRA: return out << "\"ULTRA\"";
    }
    return out << "\"INVALID\"";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, BlendMode* out) {
    if (0 == compare(tokens[i], jsonChunk, "OPAQUE")) { *out = BlendMode::OPAQUE; }
    else if (0 == compare(tokens[i], jsonChunk, "TRANSLUCENT")) { *out = BlendMode::TRANSLUCENT; }
    else {
        slog.w << "Invalid BlendMode: '" << STR(tokens[i], jsonChunk) << "'" << io::endl;
    }
    return i + 1;
}

std::ostream& operator<<(std::ostream& out, BlendMode in) {
    switch (in) {
        case BlendMode::OPAQUE: return out << "\"OPAQUE\"";
        case BlendMode::TRANSLUCENT: return out << "\"TRANSLUCENT\"";
    }
    return out << "\"INVALID\"";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, DynamicResolutionOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "minScale") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->minScale);
        } else if (compare(tok, jsonChunk, "maxScale") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->maxScale);
        } else if (compare(tok, jsonChunk, "sharpness") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->sharpness);
        } else if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else if (compare(tok, jsonChunk, "homogeneousScaling") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->homogeneousScaling);
        } else if (compare(tok, jsonChunk, "quality") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->quality);
        } else {
            slog.w << "Invalid DynamicResolutionOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid DynamicResolutionOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const DynamicResolutionOptions& in) {
    return out << "{\n"
        << "\"minScale\": " << (in.minScale) << ",\n"
        << "\"maxScale\": " << (in.maxScale) << ",\n"
        << "\"sharpness\": " << (in.sharpness) << ",\n"
        << "\"enabled\": " << to_string(in.enabled) << ",\n"
        << "\"homogeneousScaling\": " << to_string(in.homogeneousScaling) << ",\n"
        << "\"quality\": " << (in.quality) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, BloomOptions::BlendMode* out) {
    if (0 == compare(tokens[i], jsonChunk, "ADD")) { *out = BloomOptions::BlendMode::ADD; }
    else if (0 == compare(tokens[i], jsonChunk, "INTERPOLATE")) { *out = BloomOptions::BlendMode::INTERPOLATE; }
    else {
        slog.w << "Invalid BloomOptions::BlendMode: '" << STR(tokens[i], jsonChunk) << "'" << io::endl;
    }
    return i + 1;
}

std::ostream& operator<<(std::ostream& out, BloomOptions::BlendMode in) {
    switch (in) {
        case BloomOptions::BlendMode::ADD: return out << "\"ADD\"";
        case BloomOptions::BlendMode::INTERPOLATE: return out << "\"INTERPOLATE\"";
    }
    return out << "\"INVALID\"";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, BloomOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "dirt") == 0) {
            // JSON serialization for dirt is not supported.
            int unused;
            i = parse(tokens, i + 1, jsonChunk, &unused);
        } else if (compare(tok, jsonChunk, "dirtStrength") == 0) {
            // JSON serialization for dirtStrength is not supported.
            int unused;
            i = parse(tokens, i + 1, jsonChunk, &unused);
        } else if (compare(tok, jsonChunk, "strength") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->strength);
        } else if (compare(tok, jsonChunk, "resolution") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->resolution);
        } else if (compare(tok, jsonChunk, "levels") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->levels);
        } else if (compare(tok, jsonChunk, "blendMode") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->blendMode);
        } else if (compare(tok, jsonChunk, "threshold") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->threshold);
        } else if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else if (compare(tok, jsonChunk, "highlight") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->highlight);
        } else if (compare(tok, jsonChunk, "quality") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->quality);
        } else if (compare(tok, jsonChunk, "lensFlare") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->lensFlare);
        } else if (compare(tok, jsonChunk, "starburst") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->starburst);
        } else if (compare(tok, jsonChunk, "chromaticAberration") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->chromaticAberration);
        } else if (compare(tok, jsonChunk, "ghostCount") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->ghostCount);
        } else if (compare(tok, jsonChunk, "ghostSpacing") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->ghostSpacing);
        } else if (compare(tok, jsonChunk, "ghostThreshold") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->ghostThreshold);
        } else if (compare(tok, jsonChunk, "haloThickness") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->haloThickness);
        } else if (compare(tok, jsonChunk, "haloRadius") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->haloRadius);
        } else if (compare(tok, jsonChunk, "haloThreshold") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->haloThreshold);
        } else {
            slog.w << "Invalid BloomOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid BloomOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const BloomOptions& in) {
    return out << "{\n"
        // JSON serialization for dirt is not supported.
        // JSON serialization for dirtStrength is not supported.
        << "\"strength\": " << (in.strength) << ",\n"
        << "\"resolution\": " << (in.resolution) << ",\n"
        << "\"levels\": " << int(in.levels) << ",\n"
        << "\"blendMode\": " << (in.blendMode) << ",\n"
        << "\"threshold\": " << to_string(in.threshold) << ",\n"
        << "\"enabled\": " << to_string(in.enabled) << ",\n"
        << "\"highlight\": " << (in.highlight) << ",\n"
        << "\"quality\": " << (in.quality) << ",\n"
        << "\"lensFlare\": " << to_string(in.lensFlare) << ",\n"
        << "\"starburst\": " << to_string(in.starburst) << ",\n"
        << "\"chromaticAberration\": " << (in.chromaticAberration) << ",\n"
        << "\"ghostCount\": " << int(in.ghostCount) << ",\n"
        << "\"ghostSpacing\": " << (in.ghostSpacing) << ",\n"
        << "\"ghostThreshold\": " << (in.ghostThreshold) << ",\n"
        << "\"haloThickness\": " << (in.haloThickness) << ",\n"
        << "\"haloRadius\": " << (in.haloRadius) << ",\n"
        << "\"haloThreshold\": " << (in.haloThreshold) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, FogOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "distance") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->distance);
        } else if (compare(tok, jsonChunk, "cutOffDistance") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->cutOffDistance);
        } else if (compare(tok, jsonChunk, "maximumOpacity") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->maximumOpacity);
        } else if (compare(tok, jsonChunk, "height") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->height);
        } else if (compare(tok, jsonChunk, "heightFalloff") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->heightFalloff);
        } else if (compare(tok, jsonChunk, "color") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->color);
        } else if (compare(tok, jsonChunk, "density") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->density);
        } else if (compare(tok, jsonChunk, "inScatteringStart") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->inScatteringStart);
        } else if (compare(tok, jsonChunk, "inScatteringSize") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->inScatteringSize);
        } else if (compare(tok, jsonChunk, "fogColorFromIbl") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->fogColorFromIbl);
        } else if (compare(tok, jsonChunk, "skyColor") == 0) {
            // JSON serialization for skyColor is not supported.
            int unused;
            i = parse(tokens, i + 1, jsonChunk, &unused);
        } else if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else {
            slog.w << "Invalid FogOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid FogOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const FogOptions& in) {
    return out << "{\n"
        << "\"distance\": " << (in.distance) << ",\n"
        << "\"cutOffDistance\": " << (std::isnan(in.cutOffDistance) ? "\"NAN\"" : std::isinf(in.cutOffDistance) ? "\"INFINITY\"" : std::to_string(in.cutOffDistance)) << ",\n"
        << "\"maximumOpacity\": " << (in.maximumOpacity) << ",\n"
        << "\"height\": " << (in.height) << ",\n"
        << "\"heightFalloff\": " << (in.heightFalloff) << ",\n"
        << "\"color\": " << (in.color) << ",\n"
        << "\"density\": " << (in.density) << ",\n"
        << "\"inScatteringStart\": " << (in.inScatteringStart) << ",\n"
        << "\"inScatteringSize\": " << (in.inScatteringSize) << ",\n"
        << "\"fogColorFromIbl\": " << to_string(in.fogColorFromIbl) << ",\n"
        // JSON serialization for skyColor is not supported.
        << "\"enabled\": " << to_string(in.enabled) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, DepthOfFieldOptions::Filter* out) {
    if (0 == compare(tokens[i], jsonChunk, "NONE")) { *out = DepthOfFieldOptions::Filter::NONE; }
    else if (0 == compare(tokens[i], jsonChunk, "UNUSED")) { *out = DepthOfFieldOptions::Filter::UNUSED; }
    else if (0 == compare(tokens[i], jsonChunk, "MEDIAN")) { *out = DepthOfFieldOptions::Filter::MEDIAN; }
    else {
        slog.w << "Invalid DepthOfFieldOptions::Filter: '" << STR(tokens[i], jsonChunk) << "'" << io::endl;
    }
    return i + 1;
}

std::ostream& operator<<(std::ostream& out, DepthOfFieldOptions::Filter in) {
    switch (in) {
        case DepthOfFieldOptions::Filter::NONE: return out << "\"NONE\"";
        case DepthOfFieldOptions::Filter::UNUSED: return out << "\"UNUSED\"";
        case DepthOfFieldOptions::Filter::MEDIAN: return out << "\"MEDIAN\"";
    }
    return out << "\"INVALID\"";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, DepthOfFieldOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "cocScale") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->cocScale);
        } else if (compare(tok, jsonChunk, "maxApertureDiameter") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->maxApertureDiameter);
        } else if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else if (compare(tok, jsonChunk, "filter") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->filter);
        } else if (compare(tok, jsonChunk, "nativeResolution") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->nativeResolution);
        } else if (compare(tok, jsonChunk, "foregroundRingCount") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->foregroundRingCount);
        } else if (compare(tok, jsonChunk, "backgroundRingCount") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->backgroundRingCount);
        } else if (compare(tok, jsonChunk, "fastGatherRingCount") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->fastGatherRingCount);
        } else if (compare(tok, jsonChunk, "maxForegroundCOC") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->maxForegroundCOC);
        } else if (compare(tok, jsonChunk, "maxBackgroundCOC") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->maxBackgroundCOC);
        } else {
            slog.w << "Invalid DepthOfFieldOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid DepthOfFieldOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const DepthOfFieldOptions& in) {
    return out << "{\n"
        << "\"cocScale\": " << (in.cocScale) << ",\n"
        << "\"maxApertureDiameter\": " << (in.maxApertureDiameter) << ",\n"
        << "\"enabled\": " << to_string(in.enabled) << ",\n"
        << "\"filter\": " << (in.filter) << ",\n"
        << "\"nativeResolution\": " << to_string(in.nativeResolution) << ",\n"
        << "\"foregroundRingCount\": " << int(in.foregroundRingCount) << ",\n"
        << "\"backgroundRingCount\": " << int(in.backgroundRingCount) << ",\n"
        << "\"fastGatherRingCount\": " << int(in.fastGatherRingCount) << ",\n"
        << "\"maxForegroundCOC\": " << (in.maxForegroundCOC) << ",\n"
        << "\"maxBackgroundCOC\": " << (in.maxBackgroundCOC) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, VignetteOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "midPoint") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->midPoint);
        } else if (compare(tok, jsonChunk, "roundness") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->roundness);
        } else if (compare(tok, jsonChunk, "feather") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->feather);
        } else if (compare(tok, jsonChunk, "color") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->color);
        } else if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else {
            slog.w << "Invalid VignetteOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid VignetteOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const VignetteOptions& in) {
    return out << "{\n"
        << "\"midPoint\": " << (in.midPoint) << ",\n"
        << "\"roundness\": " << (in.roundness) << ",\n"
        << "\"feather\": " << (in.feather) << ",\n"
        << "\"color\": " << (in.color) << ",\n"
        << "\"enabled\": " << to_string(in.enabled) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, RenderQuality* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "hdrColorBuffer") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->hdrColorBuffer);
        } else {
            slog.w << "Invalid RenderQuality key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid RenderQuality value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const RenderQuality& in) {
    return out << "{\n"
        << "\"hdrColorBuffer\": " << (in.hdrColorBuffer) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, AmbientOcclusionOptions::Ssct* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "lightConeRad") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->lightConeRad);
        } else if (compare(tok, jsonChunk, "shadowDistance") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->shadowDistance);
        } else if (compare(tok, jsonChunk, "contactDistanceMax") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->contactDistanceMax);
        } else if (compare(tok, jsonChunk, "intensity") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->intensity);
        } else if (compare(tok, jsonChunk, "lightDirection") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->lightDirection);
        } else if (compare(tok, jsonChunk, "depthBias") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->depthBias);
        } else if (compare(tok, jsonChunk, "depthSlopeBias") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->depthSlopeBias);
        } else if (compare(tok, jsonChunk, "sampleCount") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->sampleCount);
        } else if (compare(tok, jsonChunk, "rayCount") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->rayCount);
        } else if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else {
            slog.w << "Invalid Ssct key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid Ssct value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const AmbientOcclusionOptions::Ssct& in) {
    return out << "{\n"
        << "\"lightConeRad\": " << (in.lightConeRad) << ",\n"
        << "\"shadowDistance\": " << (in.shadowDistance) << ",\n"
        << "\"contactDistanceMax\": " << (in.contactDistanceMax) << ",\n"
        << "\"intensity\": " << (in.intensity) << ",\n"
        << "\"lightDirection\": " << (in.lightDirection) << ",\n"
        << "\"depthBias\": " << (in.depthBias) << ",\n"
        << "\"depthSlopeBias\": " << (in.depthSlopeBias) << ",\n"
        << "\"sampleCount\": " << int(in.sampleCount) << ",\n"
        << "\"rayCount\": " << int(in.rayCount) << ",\n"
        << "\"enabled\": " << to_string(in.enabled) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, AmbientOcclusionOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "radius") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->radius);
        } else if (compare(tok, jsonChunk, "power") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->power);
        } else if (compare(tok, jsonChunk, "bias") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->bias);
        } else if (compare(tok, jsonChunk, "resolution") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->resolution);
        } else if (compare(tok, jsonChunk, "intensity") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->intensity);
        } else if (compare(tok, jsonChunk, "bilateralThreshold") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->bilateralThreshold);
        } else if (compare(tok, jsonChunk, "quality") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->quality);
        } else if (compare(tok, jsonChunk, "lowPassFilter") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->lowPassFilter);
        } else if (compare(tok, jsonChunk, "upsampling") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->upsampling);
        } else if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else if (compare(tok, jsonChunk, "bentNormals") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->bentNormals);
        } else if (compare(tok, jsonChunk, "minHorizonAngleRad") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->minHorizonAngleRad);
        } else if (compare(tok, jsonChunk, "ssct") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->ssct);
        } else {
            slog.w << "Invalid AmbientOcclusionOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid AmbientOcclusionOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const AmbientOcclusionOptions& in) {
    return out << "{\n"
        << "\"radius\": " << (in.radius) << ",\n"
        << "\"power\": " << (in.power) << ",\n"
        << "\"bias\": " << (in.bias) << ",\n"
        << "\"resolution\": " << (in.resolution) << ",\n"
        << "\"intensity\": " << (in.intensity) << ",\n"
        << "\"bilateralThreshold\": " << (in.bilateralThreshold) << ",\n"
        << "\"quality\": " << (in.quality) << ",\n"
        << "\"lowPassFilter\": " << (in.lowPassFilter) << ",\n"
        << "\"upsampling\": " << (in.upsampling) << ",\n"
        << "\"enabled\": " << to_string(in.enabled) << ",\n"
        << "\"bentNormals\": " << to_string(in.bentNormals) << ",\n"
        << "\"minHorizonAngleRad\": " << (in.minHorizonAngleRad) << ",\n"
        << "\"ssct\": " << (in.ssct) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, MultiSampleAntiAliasingOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else if (compare(tok, jsonChunk, "sampleCount") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->sampleCount);
        } else if (compare(tok, jsonChunk, "customResolve") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->customResolve);
        } else {
            slog.w << "Invalid MultiSampleAntiAliasingOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid MultiSampleAntiAliasingOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const MultiSampleAntiAliasingOptions& in) {
    return out << "{\n"
        << "\"enabled\": " << to_string(in.enabled) << ",\n"
        << "\"sampleCount\": " << int(in.sampleCount) << ",\n"
        << "\"customResolve\": " << to_string(in.customResolve) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, TemporalAntiAliasingOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "filterWidth") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->filterWidth);
        } else if (compare(tok, jsonChunk, "feedback") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->feedback);
        } else if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else {
            slog.w << "Invalid TemporalAntiAliasingOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid TemporalAntiAliasingOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const TemporalAntiAliasingOptions& in) {
    return out << "{\n"
        << "\"filterWidth\": " << (in.filterWidth) << ",\n"
        << "\"feedback\": " << (in.feedback) << ",\n"
        << "\"enabled\": " << to_string(in.enabled) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, ScreenSpaceReflectionsOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "thickness") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->thickness);
        } else if (compare(tok, jsonChunk, "bias") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->bias);
        } else if (compare(tok, jsonChunk, "maxDistance") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->maxDistance);
        } else if (compare(tok, jsonChunk, "stride") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->stride);
        } else if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else {
            slog.w << "Invalid ScreenSpaceReflectionsOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid ScreenSpaceReflectionsOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const ScreenSpaceReflectionsOptions& in) {
    return out << "{\n"
        << "\"thickness\": " << (in.thickness) << ",\n"
        << "\"bias\": " << (in.bias) << ",\n"
        << "\"maxDistance\": " << (in.maxDistance) << ",\n"
        << "\"stride\": " << (in.stride) << ",\n"
        << "\"enabled\": " << to_string(in.enabled) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, GuardBandOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else {
            slog.w << "Invalid GuardBandOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid GuardBandOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const GuardBandOptions& in) {
    return out << "{\n"
        << "\"enabled\": " << to_string(in.enabled) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, AntiAliasing* out) {
    if (0 == compare(tokens[i], jsonChunk, "NONE")) { *out = AntiAliasing::NONE; }
    else if (0 == compare(tokens[i], jsonChunk, "FXAA")) { *out = AntiAliasing::FXAA; }
    else {
        slog.w << "Invalid AntiAliasing: '" << STR(tokens[i], jsonChunk) << "'" << io::endl;
    }
    return i + 1;
}

std::ostream& operator<<(std::ostream& out, AntiAliasing in) {
    switch (in) {
        case AntiAliasing::NONE: return out << "\"NONE\"";
        case AntiAliasing::FXAA: return out << "\"FXAA\"";
    }
    return out << "\"INVALID\"";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, Dithering* out) {
    if (0 == compare(tokens[i], jsonChunk, "NONE")) { *out = Dithering::NONE; }
    else if (0 == compare(tokens[i], jsonChunk, "TEMPORAL")) { *out = Dithering::TEMPORAL; }
    else {
        slog.w << "Invalid Dithering: '" << STR(tokens[i], jsonChunk) << "'" << io::endl;
    }
    return i + 1;
}

std::ostream& operator<<(std::ostream& out, Dithering in) {
    switch (in) {
        case Dithering::NONE: return out << "\"NONE\"";
        case Dithering::TEMPORAL: return out << "\"TEMPORAL\"";
    }
    return out << "\"INVALID\"";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, ShadowType* out) {
    if (0 == compare(tokens[i], jsonChunk, "PCF")) { *out = ShadowType::PCF; }
    else if (0 == compare(tokens[i], jsonChunk, "VSM")) { *out = ShadowType::VSM; }
    else if (0 == compare(tokens[i], jsonChunk, "DPCF")) { *out = ShadowType::DPCF; }
    else if (0 == compare(tokens[i], jsonChunk, "PCSS")) { *out = ShadowType::PCSS; }
    else if (0 == compare(tokens[i], jsonChunk, "PCFd")) { *out = ShadowType::PCFd; }
    else {
        slog.w << "Invalid ShadowType: '" << STR(tokens[i], jsonChunk) << "'" << io::endl;
    }
    return i + 1;
}

std::ostream& operator<<(std::ostream& out, ShadowType in) {
    switch (in) {
        case ShadowType::PCF: return out << "\"PCF\"";
        case ShadowType::VSM: return out << "\"VSM\"";
        case ShadowType::DPCF: return out << "\"DPCF\"";
        case ShadowType::PCSS: return out << "\"PCSS\"";
        case ShadowType::PCFd: return out << "\"PCFd\"";
    }
    return out << "\"INVALID\"";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, VsmShadowOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "anisotropy") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->anisotropy);
        } else if (compare(tok, jsonChunk, "mipmapping") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->mipmapping);
        } else if (compare(tok, jsonChunk, "msaaSamples") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->msaaSamples);
        } else if (compare(tok, jsonChunk, "highPrecision") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->highPrecision);
        } else if (compare(tok, jsonChunk, "minVarianceScale") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->minVarianceScale);
        } else if (compare(tok, jsonChunk, "lightBleedReduction") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->lightBleedReduction);
        } else {
            slog.w << "Invalid VsmShadowOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid VsmShadowOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const VsmShadowOptions& in) {
    return out << "{\n"
        << "\"anisotropy\": " << int(in.anisotropy) << ",\n"
        << "\"mipmapping\": " << to_string(in.mipmapping) << ",\n"
        << "\"msaaSamples\": " << int(in.msaaSamples) << ",\n"
        << "\"highPrecision\": " << to_string(in.highPrecision) << ",\n"
        << "\"minVarianceScale\": " << (in.minVarianceScale) << ",\n"
        << "\"lightBleedReduction\": " << (in.lightBleedReduction) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, SoftShadowOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "penumbraScale") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->penumbraScale);
        } else if (compare(tok, jsonChunk, "penumbraRatioScale") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->penumbraRatioScale);
        } else {
            slog.w << "Invalid SoftShadowOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid SoftShadowOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const SoftShadowOptions& in) {
    return out << "{\n"
        << "\"penumbraScale\": " << (in.penumbraScale) << ",\n"
        << "\"penumbraRatioScale\": " << (in.penumbraRatioScale) << "\n"
        << "}";
}

int parse(jsmntok_t const* tokens, int i, const char* jsonChunk, StereoscopicOptions* out) {
    CHECK_TOKTYPE(tokens[i], JSMN_OBJECT);
    int size = tokens[i++].size;
    for (int j = 0; j < size; ++j) {
        const jsmntok_t tok = tokens[i];
        CHECK_KEY(tok);
        if (compare(tok, jsonChunk, "enabled") == 0) {
            i = parse(tokens, i + 1, jsonChunk, &out->enabled);
        } else {
            slog.w << "Invalid StereoscopicOptions key: '" << STR(tok, jsonChunk) << "'" << io::endl;
            i = parse(tokens, i + 1);
        }
        if (i < 0) {
            slog.e << "Invalid StereoscopicOptions value: '" << STR(tok, jsonChunk) << "'" << io::endl;
            return i;
        }
    }
    return i;
}

std::ostream& operator<<(std::ostream& out, const StereoscopicOptions& in) {
    return out << "{\n"
        << "\"enabled\": " << to_string(in.enabled) << "\n"
        << "}";
}

} // namespace filament::viewer
