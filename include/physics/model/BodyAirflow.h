#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct AirEnvironment {
    glm::vec3 wind{0};
    float density = 1.2f;
    float dynamicViscosity = 1.8e-5f;
};

enum class AirLoadKind { Force, Drag, Lift, Propeller, Plate };
struct AirLoad {
    AirLoadKind kind;
    glm::vec3 offset; // metres from the centre of mass in principal body axes
    glm::vec3 axis;   // unit vector, body axes
    glm::vec3 parameters;
    std::vector<glm::vec3> coefficients; // J, CT, CP at the authored rpm
};

struct AirLoadVector {
    glm::vec3 offsetWorld, forceWorld;
};

struct AirflowResult {
    glm::vec3 force{0}, moment{0}; // world coordinates, about the centre of mass
    float shaftPower = 0, thrust = 0, shaftTorque = 0;
    bool withinRange = true;
};

struct DuctOperatingPoint {
    unsigned intersections = 0;
    float flow = 0, pressure = 0;
    bool coincidentInterval = false;
    bool unique() const { return intersections == 1 && !coincidentInterval; }
};

class BodyAirflow {
public:
    glm::vec3 principalInertia{0};
    glm::vec3 freeTorque{0};
    glm::vec3 forceRamp{0, 0, 1}; // start, end (seconds), final gain
    std::vector<AirLoad> loads;
    std::vector<glm::vec3> ductStations; // flow, pressure rise, system loss

    AirflowResult evaluate(const AirEnvironment &air, const glm::mat3 &bodyToWorld,
                           glm::vec3 velocity, glm::vec3 angularVelocity,
                           float seconds, std::vector<AirLoadVector> *vectors = nullptr) const;
    DuctOperatingPoint ductOperatingPoint() const;
};

// Assembly state for declarations belonging to the next I(...) action.
class AirflowDeclaration {
public:
    static bool recognizes(const std::string &name);
    std::string append(const std::string &name, glm::vec3 values);
    std::string checkBody() const;
    bool active = false;
    BodyAirflow body;

private:
    glm::vec3 offset_{0}, axis_{1, 0, 0};
    std::vector<glm::vec3> samples_;
};
