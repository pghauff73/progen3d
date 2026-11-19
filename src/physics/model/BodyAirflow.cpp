#include "physics/model/BodyAirflow.h"

#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {
bool finite(glm::vec3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool validInertia(glm::vec3 v) {
    const double largest = std::max({v.x, v.y, v.z});
    return v.x > 0 && v.y > 0 && v.z > 0 &&
           2 * largest <= double(v.x) + v.y + v.z + largest * 1e-6;
}

bool samplePropeller(const std::vector<glm::vec3> &rows, float advance, glm::vec3 &sample) {
    if (rows.size() < 2 || advance < rows.front().x || advance > rows.back().x) return false;
    const auto hi = std::lower_bound(rows.begin(), rows.end(), advance,
        [](const glm::vec3 &row, float value) { return row.x < value; });
    if (hi == rows.begin() || hi->x == advance) sample = *hi;
    else {
        const auto &lo = *(hi - 1);
        sample = glm::mix(lo, *hi, (advance - lo.x) / (hi->x - lo.x));
    }
    return true;
}
}

bool AirflowDeclaration::recognizes(const std::string &name) {
    return name == "BodyInertia" || name == "LoadPoint" || name == "LoadAxis" ||
           name == "BodyForce" || name == "BodyTorque" || name == "DragSurface" ||
           name == "LiftSurface" || name == "PropSample" || name == "Propeller" ||
           name == "DuctStation" || name == "PlateFriction" || name == "ForceRamp";
}

std::string AirflowDeclaration::append(const std::string &name, glm::vec3 v) {
    if (!finite(v)) return name + " requires finite values.";
    active = true;
    if (name == "BodyInertia") {
        if (!validInertia(v)) return "BodyInertia requires positive principal moments satisfying the triangle inequality.";
        body.principalInertia = v;
    } else if (name == "LoadPoint") {
        offset_ = v;
    } else if (name == "LoadAxis") {
        const float length = glm::length(v);
        if (!(length > 0) || !std::isfinite(length)) return "LoadAxis requires a finite nonzero direction.";
        axis_ = v / length;
    } else if (name == "ForceRamp") {
        if (v.x < 0 || v.y <= v.x) return "ForceRamp requires 0 <= start < end in seconds.";
        body.forceRamp = v;
    } else if (name == "BodyForce") {
        body.loads.push_back({AirLoadKind::Force, offset_, axis_, v, {}});
    } else if (name == "BodyTorque") {
        body.freeTorque += v;
    } else if (name == "DragSurface" || name == "LiftSurface") {
        if ((name == "DragSurface" && v.x < 0) || v.y <= 0 || v.z <= 0)
            return name + " requires a valid coefficient and positive reference area and length.";
        body.loads.push_back({name == "DragSurface" ? AirLoadKind::Drag : AirLoadKind::Lift,
                              offset_, axis_, v, {}});
    } else if (name == "PropSample") {
        if ((!samples_.empty() && v.x <= samples_.back().x) || v.z < 0)
            return "PropSample requires increasing advance ratios and nonnegative power coefficients.";
        samples_.push_back(v);
    } else if (name == "Propeller") {
        if (samples_.size() < 2 || v.x <= 0 || v.y < 0 || std::abs(v.z) != 1)
            return "Propeller requires at least two samples, positive diameter, nonnegative rpm and spin +1 or -1.";
        body.loads.push_back({AirLoadKind::Propeller, offset_, axis_, v, std::move(samples_)});
        samples_.clear();
    } else if (name == "DuctStation") {
        if (v.x < 0 || (!body.ductStations.empty() && v.x <= body.ductStations.back().x))
            return "DuctStation requires nonnegative, strictly increasing volume flow.";
        body.ductStations.push_back(v);
    } else if (name == "PlateFriction") {
        if (v.x <= 0 || v.y <= 0 || (v.z != 0 && v.z != 1 && v.z != 2))
            return "PlateFriction requires wetted area, length and regime 0, 1 or 2.";
        body.loads.push_back({AirLoadKind::Plate, offset_, axis_, v, {}});
    } else return "Unknown airflow declaration.";
    return {};
}

std::string AirflowDeclaration::checkBody() const {
    if (!active) return {};
    if (!validInertia(body.principalInertia)) return "Specify BodyInertia before the airflow body's I action.";
    if (!samples_.empty()) return "PropSample rows require a following Propeller on this body.";
    if (body.ductStations.size() == 1) return "DuctStation requires at least two flow stations.";
    return {};
}

AirflowResult BodyAirflow::evaluate(const AirEnvironment &air, const glm::mat3 &basis,
                                    glm::vec3 velocity, glm::vec3 omega,
                                    float seconds, std::vector<AirLoadVector> *vectors) const {
    AirflowResult result;
    const float gain = forceRamp.y > forceRamp.x ?
        forceRamp.z * std::clamp((seconds-forceRamp.x)/(forceRamp.y-forceRamp.x), 0.f, 1.f) : 1.f;
    result.moment = gain * (basis * freeTorque);
    for (const auto &load : loads) {
        const glm::vec3 r = basis * load.offset;
        const glm::vec3 axis = basis * load.axis;
        const glm::vec3 relative = velocity + glm::cross(omega, r) - air.wind;
        const auto p = load.parameters;
        glm::vec3 force{0};
        if (load.kind == AirLoadKind::Force) {
            force = gain * (basis * p);
        } else if (load.kind == AirLoadKind::Propeller) {
            if (p.y == 0) continue;
            const float n = p.y / 60;
            glm::vec3 sample;
            if (!samplePropeller(load.coefficients, glm::dot(relative, axis) / (n * p.x), sample)) {
                result.withinRange = false;
                continue;
            }
            const float scale = air.density * n * n * p.x * p.x * p.x * p.x;
            const float thrust = scale * sample.y;
            const float power = scale * n * p.x * sample.z;
            const float torque = power / (glm::two_pi<float>() * n);
            force = thrust * axis;
            result.thrust += thrust;
            result.shaftPower += power;
            result.shaftTorque += p.z * torque;
            result.moment -= p.z * torque * axis;
        } else {
            const float speed = glm::length(relative);
            if (speed == 0) continue;
            const float q = 0.5f * air.density * speed * speed;
            const glm::vec3 direction = relative / speed;
            if (load.kind == AirLoadKind::Lift) {
                const glm::vec3 normal = axis - glm::dot(axis, direction) * direction;
                const float length = glm::length(normal);
                if (length > 1e-6f) force = (q * p.x * p.y / length) * normal;
            } else if (load.kind == AirLoadKind::Drag) {
                force = -q * p.x * p.y * direction;
            } else {
                // Edge-on plate; the author supplies tangential incident flow and one wetted area.
                const float reynolds = air.density * speed * p.y / air.dynamicViscosity;
                float cf = 1.328f / std::sqrt(reynolds);
                if (p.z == 2 || (p.z == 1 && reynolds > 5e5f)) {
                    if (reynolds < 5e5f || reynolds > 1e7f) {
                        result.withinRange = false;
                        continue;
                    }
                    cf = 0.074f / std::pow(reynolds, 0.2f);
                    if (p.z == 1) cf -= 1742 / reynolds;
                }
                force = -q * p.x * cf * direction;
            }
        }
        if (vectors) vectors->push_back({r, force});
        result.force += force;
        result.moment += glm::cross(r, force);
    }
    return result;
}

DuctOperatingPoint BodyAirflow::ductOperatingPoint() const {
    DuctOperatingPoint result;
    for (std::size_t i = 0; i < ductStations.size(); ++i) {
        const auto row = ductStations[i];
        const float residual = row.y - row.z;
        if (residual == 0) {
            ++result.intersections;
            result.flow = row.x;
            result.pressure = row.y;
        }
        if (i == 0) continue;
        const auto prev = ductStations[i-1];
        const float previous = prev.y - prev.z;
        if (residual == 0 && previous == 0) result.coincidentInterval = true;
        if ((residual < 0 && previous > 0) || (residual > 0 && previous < 0)) {
            const float t = previous / (previous - residual);
            result.flow = glm::mix(prev.x, row.x, t);
            result.pressure = glm::mix(prev.y, row.y, t);
            ++result.intersections;
        }
    }
    return result;
}
