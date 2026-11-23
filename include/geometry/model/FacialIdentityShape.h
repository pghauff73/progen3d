#pragma once

// Twenty-four dimensionless identity coefficients in persistent face-material coordinates.
// Zero is exactly the historical neutral surface. Every nonzero shape is rebuilt before
// membrane/bending rest assembly; a solved elastic mesh is never post-warped.
#include <array>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

struct FacialIdentityShape {
    std::array<float, 24> coefficients{};

    bool active() const {
        for (const float value : coefficients) if (value != 0.0f) return true;
        return false;
    }
};

// This is a *rest-shape* basis, not an anatomical calibration. Coordinate convention:
// u,v in [-1,1], positive u is anatomical left, +Y up, +Z forward.
// Layout: eight width-v knots, eight front-depth-v knots, cheekL, cheekR,
// nose tip, bridge, lips, orbital spacing, nose vertical, mouth vertical.
inline glm::vec3 applyFacialIdentityShape(
    glm::vec3 point, float u, float v, const glm::vec3 &size,
    const FacialIdentityShape &identity)
{
    if (!identity.active()) return point; // preserve zero-identity buffers bit for bit
    const auto &theta = identity.coefficients;
    constexpr std::array<float, 8> knots{{-1.f, -.72f, -.45f, -.18f, .10f, .37f, .66f, 1.f}};
    float weights[8] = {}, total = 0.f;
    for (unsigned k = 0; k < 8; ++k) {
        const float distance = (v - knots[k]) / .19f;
        weights[k] = std::exp(-.5f * distance * distance);
        total += weights[k];
    }
    float width = 0.f, depth = 0.f;
    for (unsigned k = 0; k < 8; ++k) {
        const float fraction = weights[k] / total;
        width += fraction * theta[k];
        depth += fraction * theta[k + 8];
    }
    auto field = [u, v](float x, float y, float rx, float ry) {
        const float a = (u - x) / rx, b = (v - y) / ry;
        return std::exp(-(a * a + b * b));
    };
    const float eye_left = field(5.f/12.f, 5.f/16.f, .17f, .16f);
    const float eye_right = field(-5.f/12.f, 5.f/16.f, .17f, .16f);
    const float nose_tip = field(0.f, -.065f, .13f, .18f);
    const float nose_bridge = field(0.f, .185f, .10f, .30f);
    const float lips = field(0.f, -.375f, .39f, .15f);
    const float mouth = field(0.f, -.375f, .34f, .19f);
    const glm::vec3 neutral_point = point;
    point.x += .5f * size.x * (u * width + theta[21] * (eye_left - eye_right));
    point.y += .5f * size.y * (theta[22] * nose_tip + theta[23] * mouth);
    point.z += size.z * (std::sqrt(std::max(0.f, 1.f - u*u)) * depth +
        theta[16] * field(.51f, -.03f, .34f, .32f) +
        theta[17] * field(-.51f, -.03f, .34f, .32f) +
        theta[18] * nose_tip + theta[19] * nose_bridge + theta[20] * lips);
    // Rounded-square material corners are poorly conditioned.
    // Smoothly eliminate identity displacement in their limiting cells.
    auto smooth_corner = [](float value,float lo,float hi) {
        const float f=std::clamp((std::fabs(value)-lo)/(hi-lo),0.0f,1.0f);
        return f*f*(3.0f-2.0f*f);
    };
    const float taper=1.0f-smooth_corner(u,.89f,.955f)*smooth_corner(v,.88f,.985f);
    return neutral_point+taper*(point-neutral_point);
}
