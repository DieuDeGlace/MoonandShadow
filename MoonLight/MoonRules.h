#pragma once
// Pure day/night rules shared by MoonLight and SteamShadows (and their tests).
#include <cmath>
#include <algorithm>
// Shared with other mods through QueryMoonLight(); keep the layout stable.
struct MoonState {
    unsigned version = 1;
    float weight = 0, direction[3]{}, shadowStrength = .20f;
};
namespace moonrules {
inline float smooth(float x) {
    x = std::clamp(x, 0.f, 1.f);
    return x * x * (3 - 2 * x);
}
// Moon for a game time in milliseconds (24 h cycle). The moon rises at 18:00
// and sets at 06:00; `weight` fades in/out over the first/last two hours, and
// `direction` is a unit vector towards the moon.
inline MoonState state(float milliseconds, float shadowStrength = .20f) {
    MoonState s;
    s.shadowStrength = shadowStrength;
    if (!std::isfinite(milliseconds))
        return s;
    float hour = std::fmod(milliseconds / 3600000.f, 24.f);
    if (hour < 0)
        hour += 24;
    s.weight = hour >= 18 ? smooth((hour - 18) / 2) : hour <= 6 ? smooth((6 - hour) / 2) : 0;
    float angle = ((hour < 18 ? hour + 24 : hour) - 18) * (3.141592653589793f / 12);
    s.direction[0] = std::cos(angle) * .94f;
    s.direction[1] = std::cos(angle) * .341174f;
    s.direction[2] = std::sin(angle);
    float n = std::sqrt(s.direction[0] * s.direction[0] + s.direction[1] * s.direction[1] +
                        s.direction[2] * s.direction[2]);
    for (float &v : s.direction)
        v /= n;
    return s;
}
// Night version of the native light buffer (12 floats): [0..2] front colour,
// [4..6] back colour, [8..10] light direction (replaced by the moon's).
inline void lighting(float out[12], const float native[12], MoonState state, float amount) {
    std::copy(native, native + 12, out);
    float w = state.weight * amount;
    const float front[] = {.32f, .38f, .5f}, back[] = {.07f, .085f, .12f};
    for (int i = 0; i < 3; i++) {
        out[i] = std::clamp(out[i] + front[i] * w, 0.f, 1.f);
        out[4 + i] = std::clamp(out[4 + i] + back[i] * w, 0.f, 1.f);
        if (state.weight > 0)
            out[8 + i] = state.direction[i];
    }
    float n = std::sqrt(out[8] * out[8] + out[9] * out[9] + out[10] * out[10]);
    if (n > .001f)
        for (int i = 8; i < 11; i++)
            out[i] /= n;
}
// Darkens and cools a cloud's material colour at night.
inline void cloud(float out[4], const float native[4], float weight, float nightScale) {
    std::copy(native, native + 4, out);
    const float tint[] = {.88f, .94f, 1.f};
    for (int i = 0; i < 3; i++)
        out[i] *= 1 - weight + weight * nightScale * tint[i];
}
} // namespace moonrules

namespace moonrules {
struct ShadowCycle {
    float direction[3]{}, strength = 0;
};
// Shadow strength factor for a light at elevation sin(angle) `z`: 0 at the
// horizon, full from about 14.5 degrees (z = 0.25) up, so shadows keep their
// configured darkness for most of the day and still fade smoothly when the
// shadow light switches between sun and moon.
inline float horizonFade(float z) {
    return smooth(z / .25f);
}
// Which light casts shadows: the sun while it is above the horizon, otherwise
// the moon; strength fades out near the horizon.
inline ShadowCycle shadows(float milliseconds, float dayStrength, float nightStrength) {
    auto moon = state(milliseconds, nightStrength);
    ShadowCycle out;
    bool day = moon.direction[2] < 0;
    for (int i = 0; i < 3; i++)
        out.direction[i] = day ? -moon.direction[i] : moon.direction[i];
    float horizon = horizonFade(out.direction[2]);
    out.strength = day ? dayStrength * horizon : nightStrength * moon.weight * horizon;
    return out;
}
} // namespace moonrules
