#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

// SI inputs. Signals are evaluated analytically at the scene clock so that
// fixed stepping, random timeline access and reset share the same solution.
class EngineeringDevicePhysics {
public:
    using Parameters = std::vector<double>;
    using Signal = std::pair<std::string, Parameters>;

    explicit EngineeringDevicePhysics(double epochSeconds = 0.0);
    static bool recognizes(const std::string& name);
    static const char* unit(const std::string& name);
    bool empty() const { return signals_.empty(); }
    double bind(const std::string& name, const Parameters& parameters);
    double timeSeconds() const { return time_; }
    void advance(double deltaSeconds);
    void reset();
    std::map<Signal, double> readings() const;

private:
    double epoch_;
    double time_;
    // A binding is immutable within a generated scene. Equal calls share it.
    std::set<Signal> signals_;
};
