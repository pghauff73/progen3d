#pragma once

#include <map>
#include <optional>
#include <string>

// All physical quantities use SI units except winding temperature and shaft speed.
struct ConductorMaterial {
    const char *name;
    double resistivity20, temperatureCoefficient;
    double resistivityAt(double celsius) const;
};
struct InsulationMaterial {
    const char *name;
    double relativePermittivity;
};
struct CoreMaterial {
    const char *name;
    double relativePermeability;
    std::optional<double> fluxDensityLimit;
};
struct PermanentMagnetMaterial {
    const char *name;
    double remanence, recoilPermeability;
};

class ElectricalMaterialCatalog {
public:
    static const ConductorMaterial *conductor(const std::string &name);
    static const InsulationMaterial *insulation(const std::string &name);
    static const CoreMaterial *core(const std::string &name);
    static const PermanentMagnetMaterial *magnet(const std::string &name);
};

struct MagnetWire {
    const ConductorMaterial *conductor;
    const InsulationMaterial *insulation;
    double diameter, insulationThickness, temperature;
    double conductorArea() const;
    double resistancePerMetre() const;
};
struct FieldCoil {
    std::string wireId;
    const CoreMaterial *core;
    unsigned turns;
    double meanTurnLength, coreLength, area, gap, current, insulationVoltage;
};
struct PermanentMagnet {
    const PermanentMagnetMaterial *material;
    const CoreMaterial *returnCore;
    double length, returnLength, area, gap;
    int polarity;
};

enum class MotorTopology { BrushedDC, RadialInrunner, RadialOutrunner, SixStepOutrunner, AxialFlux, InductionCage };
enum class WindingConnection { Armature, Wye, Delta };
std::optional<MotorTopology> findMotorTopology(const std::string &name);
const char *motorTopologyName(MotorTopology topology);
const char *windingConnectionName(WindingConnection connection);
struct RotorCage {
    const ConductorMaterial *conductor;
    unsigned bars;
    double pathLength, barArea, temperature, leakageInductance, standstillEmfRms, supplyFrequency;
};
struct ElectricMotor {
    MotorTopology topology;
    WindingConnection connection;
    std::string windingId, magnetId;
    unsigned polePairs;
    double rpm, windingFactor;
    std::optional<RotorCage> cage;
};

// Stored by value in each generated scene. Relationships use stable scene IDs.
struct ElectricalMachineModel {
    std::map<std::string, MagnetWire> wires;
    std::map<std::string, FieldCoil> coils;
    std::map<std::string, PermanentMagnet> magnets;
    std::map<std::string, ElectricMotor> motors;
};
