#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

class Mesh;

enum class FlexibleTopology { Wire, Belt, Sheet };

struct ElasticMaterial {
    double density = 1100;        // kg/m^3
    double youngModulus = 5e6;    // Pa
    double dampingRate = 1.0;     // 1/s
};

// Grammar dimensions and pins are attached to one I(...) instance.
class FlexibleBodySpecification {
public:
    static std::optional<FlexibleTopology> topologyOf(const std::string &type);
    static int argumentCount(const std::string &action);
    std::string configure(const std::string &action, glm::vec3 values);
    std::string validate(FlexibleTopology topology) const;
    glm::ivec2 gridFor(FlexibleTopology topology) const;
    glm::dvec3 dimensionsFor(FlexibleTopology topology) const;
    std::size_t particleCount(FlexibleTopology topology) const;

    ElasticMaterial material;
    double sceneUnitsPerMetre = 1; // mesh conversion only; particle positions remain SI
    std::optional<glm::dvec3> dimensions;
    std::optional<glm::ivec2> grid;
    std::optional<double> guideRadius;
    std::optional<double> initialBeltSpeed;
    std::vector<glm::ivec2> pins;
};

struct ElasticParticle {
    glm::dvec3 restPosition{0}, position{0}, velocity{0};
    double mass = 0;
    double inverseMass = 0;
};

// The particle coordinates are physical metres in the rotated object frame.
class FlexibleBody {
public:
    FlexibleBody(FlexibleTopology topology, const FlexibleBodySpecification &specification,
                 double uniformScale = 1);
    void setInitialMotion(glm::dvec3 linearVelocity, glm::dvec3 angularVelocity);
    void setTotalMass(double kilograms);
    void reset();
    void advance(double seconds, glm::dvec3 acceleration);
    Mesh surfaceMesh() const;
    FlexibleTopology topology() const { return topology_; }
    const ElasticMaterial &material() const { return material_; }
    glm::ivec2 grid() const { return grid_; }
    double totalMass() const { return mass_; }
    const std::vector<ElasticParticle> &particles() const { return particles_; }

private:
    struct StretchConstraint {
        int first, second;
        double restLength, compliance, multiplier = 0;
    };
    struct BendConstraint {
        int first, middle, last;
        double restAngle, compliance, multiplier = 0;
    };

    int index(int along, int across) const;
    void addStretch(int first, int second, double axialRigidity);
    void addBend(int first, int middle, int last, double flexuralRigidity);
    void buildWire();
    void buildRibbon();
    void solveStretch(StretchConstraint &edge, double inverseStepSquared);
    void solveBend(BendConstraint &hinge, double inverseStepSquared);
    void projectBeltGuides();
    glm::dvec3 ribbonPoint(int along, int across) const;

    FlexibleTopology topology_;
    ElasticMaterial material_;
    glm::dvec3 dimensions_;
    glm::ivec2 grid_;
    double meshUnitsPerPhysicalMetre_, guideRadius_, beltSpeed_, mass_ = 0;
    std::vector<ElasticParticle> particles_;
    std::vector<glm::dvec3> initialVelocities_, previousPositions_;
    std::vector<StretchConstraint> stretches_;
    std::vector<BendConstraint> bends_;
};
