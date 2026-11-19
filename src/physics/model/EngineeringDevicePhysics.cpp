#include "physics/model/EngineeringDevicePhysics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace {
using Parameters = EngineeringDevicePhysics::Parameters;
constexpr double pi = 3.14159265358979323846;

void require(bool condition, const char* message)
{
    if (!condition) throw std::invalid_argument(message);
}

struct ThermalContact {
    bool closed;
    double temperature;
};

ThermalContact sampleContact(double t, const Parameters& p)
{
    const auto ambient = p[0], power = p[1], capacity = p[2];
    const auto offG = p[3], onG = p[4], closeAt = p[5], openAt = p[6];
    require(ambient > 0 && power >= 0 && capacity > 0 && offG > 0 && onG >= offG
            && ambient < openAt && openAt < closeAt, "invalid thermal constants or hysteresis thresholds");
    const double openRise = power / offG, closedRise = power / onG;
    const double high = closeAt - ambient, low = openAt - ambient;
    // C dT/dt = P - G (T - ambient). Start open, at ambient.
    auto evolve = [](double start, double equilibrium, double duration, double tau) {
        return start + (equilibrium - start) * -std::expm1(-duration / tau);
    };
    const double offTau = capacity / offG, onTau = capacity / onG;
    if (openRise <= high)
        return {false, ambient + evolve(0, openRise, t, offTau)};
    const double firstClose = offTau * std::log1p(high / (openRise - high));
    if (t < firstClose)
        return {false, ambient + evolve(0, openRise, t, offTau)};
    t -= firstClose;
    if (closedRise >= low)
        return {true, ambient + evolve(high, closedRise, t, onTau)};
    const double cooling = onTau * std::log1p((high - low) / (low - closedRise));
    const double heating = offTau * std::log1p((high - low) / (openRise - high));
    const double phase = std::fmod(t, cooling + heating);
    if (phase < cooling)
        return {true, ambient + evolve(high, closedRise, phase, onTau)};
    return {false, ambient + evolve(low, openRise, phase - cooling, offTau)};
}

struct MoistureBalance {
    double stored;
    double collected;
};

MoistureBalance sampleWindow(double t, const Parameters& p)
{
    const auto wet = p[0], dry = p[1], uptakeTau = p[2], releaseTau = p[3];
    const auto wetCapacity = p[4], efficiency = p[5], clockRate = p[6];
    require(wet > 0 && dry > 0 && uptakeTau > 0 && releaseTau > 0 && wetCapacity >= 0
            && efficiency >= 0 && efficiency <= 1 && clockRate > 0, "invalid sorption cycle, capacity or collection fraction");
    const double elapsed = t * clockRate;
    const double period = wet + dry;
    const double phase = std::fmod(elapsed, period);
    // The gel starts at its cyclic steady-state minimum. The tank starts empty.
    const double wetGain = -std::expm1(-wet / uptakeTau);
    const double dryLoss = -std::expm1(-dry / releaseTau);
    const double maximum = wetCapacity * wetGain / -std::expm1(-wet / uptakeTau - dry / releaseTau);
    const double minimum = maximum * std::exp(-dry / releaseTau);
    const double previousRelease = std::floor(elapsed / period) * maximum * dryLoss;
    if (phase < wet)
        return {minimum + (wetCapacity - minimum) * -std::expm1(-phase / uptakeTau),
                efficiency * previousRelease};
    const double released = maximum * -std::expm1(-(phase - wet) / releaseTau);
    return {maximum * std::exp(-(phase - wet) / releaseTau), efficiency * (previousRelease + released)};
}

double crawlerStation(double t, const Parameters& p)
{
    const auto index = p[0], cells = p[1], pitch = p[2], period = p[3], tau = p[4];
    const auto pressure = p[5], area = p[6], stiffness = p[7], grip = p[8];
    require(cells >= 3 && cells <= 1024 && cells == std::floor(cells) && index == std::floor(index)
            && index >= 0 && index <= cells, "crawler requires an integer station in [0, cells] and 3..1024 cells");
    require(pitch > 0 && period > 0 && tau >= 0 && pressure >= 0 && area > 0
            && stiffness > 0 && std::abs(grip) <= 1, "invalid pneumatic, spring or traction parameters");
    const double omega = 2*pi / period;
    const double lag = std::atan(omega * tau);
    const double stroke = (area * pressure / stiffness) / std::hypot(1.0, omega * tau);
    require(stroke < pitch, "pneumatic stroke must be smaller than the resting cell length");
    const double amplitude = stroke / (2 * std::sin(pi / cells));
    // Quasistatic force balance on N distinct drag pads (the end station is
    // a geometric boundary). Phase-dependent drag gives the nonzero mean speed.
    const double speed = amplitude * omega * grip / 2;
    const double phase = 2*pi * (std::fmod(t, period) / period - index / cells) - lag;
    return index * pitch + amplitude * (std::cos(lag) - std::cos(phase)) + speed * t;
}

double latticeAngle(double t, const Parameters& p)
{
    const auto initial = p[0], rest = p[1], inertia = p[2], damping = p[3], stiffness = p[4], torque = p[5];
    require(inertia > 0 && damping >= 0 && stiffness > 0, "require positive inertia and stiffness and nonnegative damping");
    if (t == 0) return initial;
    const double equilibrium = rest + torque / stiffness;
    const double natural = std::sqrt(stiffness / inertia);
    const double decay = damping / (2 * inertia);
    double response;
    if (decay < natural) {
        const double freq = std::sqrt((natural - decay) * (natural + decay));
        const double phase = freq * t;
        response = std::exp(-decay*t) * (std::cos(phase) + decay * std::sin(phase) / freq);
    } else if (decay == natural) {
        response = std::exp(-decay*t) * (1 + decay*t);
    } else {
        const double root = std::sqrt((decay - natural) * (decay + natural));
        const double slow = -(natural * natural) / (decay + root);
        // Factoring out the slow root avoids both growing exponentials and
        // cancellation of the two modes near critical damping.
        response = std::exp(slow*t) * (1 + slow * std::expm1(-2*root*t) / (2*root));
    }
    return equilibrium + (initial - equilibrium) * response;
}

struct QuantityDefinition {
    const char* name;
    const char* unit;
    std::size_t arity;
    double (*sample)(double, const Parameters&);
};
const std::array<QuantityDefinition, 6> quantities{{
    {"thermalClosed", "", 7, [](double t, const Parameters& p) { return sampleContact(t, p).closed ? 1.0 : 0.0; }},
    {"thermalKelvin", "K", 7, [](double t, const Parameters& p) { return sampleContact(t, p).temperature; }},
    {"gelMass", "kg", 7, [](double t, const Parameters& p) { return sampleWindow(t, p).stored; }},
    {"collectedMass", "kg", 7, [](double t, const Parameters& p) { return sampleWindow(t, p).collected; }},
    {"crawlerStation", "m", 9, crawlerStation},
    {"latticeAngle", "rad", 6, latticeAngle},
}};
auto findQuantity(const std::string& name)
{
    return std::find_if(quantities.begin(), quantities.end(), [&](const auto& q) { return name == q.name; });
}

double sample(const std::string& name, const Parameters& p, double time)
{
    const auto quantity = findQuantity(name);
    if (quantity == quantities.end()) throw std::invalid_argument("unknown device quantity");
    if (p.size() != quantity->arity)
        throw std::invalid_argument(name + " expects " + std::to_string(quantity->arity) + " arguments");
    for (double value : p) require(std::isfinite(value), "device parameters must be finite");
    const double result = quantity->sample(std::max(0.0, time), p);
    require(std::isfinite(result), "device state exceeds the numeric range");
    return result;
}
}

EngineeringDevicePhysics::EngineeringDevicePhysics(double epochSeconds)
    : epoch_(epochSeconds), time_(epochSeconds)
{
    require(std::isfinite(epochSeconds), "scene clock must be finite");
}

bool EngineeringDevicePhysics::recognizes(const std::string& name)
{
    return findQuantity(name) != quantities.end();
}

const char* EngineeringDevicePhysics::unit(const std::string& name)
{
    const auto quantity = findQuantity(name);
    return quantity == quantities.end() ? "" : quantity->unit;
}

double EngineeringDevicePhysics::bind(const std::string& name, const Parameters& parameters)
{
    const double value = sample(name, parameters, time_);
    signals_.emplace(name, parameters);
    return value;
}

void EngineeringDevicePhysics::advance(double deltaSeconds)
{
    if (deltaSeconds > 0 && std::isfinite(time_ + deltaSeconds)) time_ += deltaSeconds;
}

void EngineeringDevicePhysics::reset()
{
    time_ = epoch_;
}

std::map<EngineeringDevicePhysics::Signal, double> EngineeringDevicePhysics::readings() const
{
    std::map<Signal, double> result;
    for (const auto& signal : signals_)
        result.emplace(signal, sample(signal.first, signal.second, time_));
    return result;
}
