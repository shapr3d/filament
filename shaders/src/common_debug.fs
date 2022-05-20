//*****************************************************************************
// Common debug facilities for "printf style debugging" in fragment shaders. 
// Use the {Get|Set}DebugColor interfaces to output vec3s from the shader.
//*****************************************************************************

#if defined(DEBUG_OUTPUT)

vec4 DEBUG_COLOR = vec4(0.0);

void SetDebugColor(vec3 col) {
    DEBUG_COLOR.rgb = col;
    DEBUG_COLOR.a = 1.0f;
}

vec3 GetDebugColor() {
    return DEBUG_COLOR.rgb;
}

bool WasDebugSet() {
    return DEBUG_COLOR.a != 0.0f;
}

#else

void SetDebugColor(vec3 col) {}
vec3 GetDebugColor() {
    return vec3(0.0);
}
bool WasDebugSet() {
    return false;
}

#endif

void SetDebugColor(float r, float g, float b) {
    SetDebugColor(vec3(r, g, b));
}