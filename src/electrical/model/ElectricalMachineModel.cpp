#include "electrical/model/ElectricalMachineModel.h"

namespace {
constexpr double pi = 3.14159265358979323846;
template<class Material, std::size_t N>
const Material *findMaterial(const Material (&catalog)[N], const std::string &name) {
    for (const auto &material : catalog) {
        if (name == material.name) return &material;
    }
    return nullptr;
}
}

double ConductorMaterial::resistivityAt(double celsius) const {
    return resistivity20 * (1 + temperatureCoefficient * (celsius - 20));
}

double MagnetWire::conductorArea() const { return pi * diameter * diameter / 4; }
double MagnetWire::resistancePerMetre() const {
    return conductor->resistivityAt(temperature) / conductorArea();
}

// Reference conditions and the explicitly assumed linear fits are in examples/ElectricalMachines.
const ConductorMaterial *ElectricalMaterialCatalog::conductor(const std::string &name) {
    static constexpr ConductorMaterial materials[] = {
        {"Copper", 1.0 / 58e6, 0.00393}, {"Aluminium", 2.826e-8, 0.004}
    };
    return findMaterial(materials, name);
}
const InsulationMaterial *ElectricalMaterialCatalog::insulation(const std::string &name) {
    static constexpr InsulationMaterial materials[] = {{"Polyimide", 3.4}, {"Air", 1}};
    return findMaterial(materials, name);
}
const CoreMaterial *ElectricalMaterialCatalog::core(const std::string &name) {
    static const CoreMaterial materials[] = {
        {"Air", 1, std::nullopt}, {"FerriteN87", 2200, 0.490}, {"LaminatedSteel", 2000, 1.5}
    };
    return findMaterial(materials, name);
}
const PermanentMagnetMaterial *ElectricalMaterialCatalog::magnet(const std::string &name) {
    static constexpr PermanentMagnetMaterial materials[] = {{"NdFeB35", 1.21, 1.05}, {"FerriteC8", 0.385, 1.10}};
    return findMaterial(materials, name);
}


namespace {
struct NamedTopology { const char *name; MotorTopology topology; };
constexpr NamedTopology topologies[] = {
    {"BrushedDC", MotorTopology::BrushedDC}, {"RadialInrunner", MotorTopology::RadialInrunner},
    {"RadialOutrunner", MotorTopology::RadialOutrunner}, {"AxialFlux", MotorTopology::AxialFlux},
    {"SixStepOutrunner", MotorTopology::SixStepOutrunner}, {"InductionCage", MotorTopology::InductionCage}
};
}
std::optional<MotorTopology> findMotorTopology(const std::string &name) {
    for (const auto &entry : topologies) if (entry.name == name) return entry.topology;
    return std::nullopt;
}
const char *motorTopologyName(MotorTopology topology) {
    for (const auto &entry : topologies) if (entry.topology == topology) return entry.name;
    return "Unknown";
}
const char *windingConnectionName(WindingConnection connection) {
    switch (connection) {
    case WindingConnection::Armature: return "Armature";
    case WindingConnection::Wye: return "Wye";
    case WindingConnection::Delta: return "Delta";
    }
    return "Unknown";
}
