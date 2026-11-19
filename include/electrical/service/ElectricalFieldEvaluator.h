#pragma once

#include "electrical/model/ElectricalMachineModel.h"

struct WindingFieldSample {
    double wireLength, resistance, conductorField, currentDensity, ohmicPower;
    double insulationField, displacement, fluxDensity, coreField, gapField;
    double flux, inductance, storedEnergy;
    bool beyondCoreLimit;
};
struct MagnetFieldSample {
    double fluxDensity, magnetField, gapField, flux;
    bool beyondCoreLimit;
};
struct MotorOperatingPoint {
    double torque, convertedPower, windingLoss, lineCurrent, electricalFrequency;
    std::optional<double> lineBackEmf, synchronousRpm, slip, rotorFrequency, rotorLoss;
    bool beyondCoreLimit;
};

struct MotorShaftSample {
    glm::vec3 worldAxis;
    double relativeRpm, inverseInertia;
    MotorOperatingPoint electrical;
};

class ElectricalFieldEvaluator {
public:
    static WindingFieldSample winding(const FieldCoil &coil, const MagnetWire &wire);
    static MagnetFieldSample magnet(const PermanentMagnet &magnet);
    static MotorOperatingPoint motorAtSpeed(const ElectricMotor &motor, const ElectricalMachineModel &scene, double rpm);
    static double shaftAngularImpulse(const ElectricMotor &motor, const MotorShaftSample &sample, double seconds);
    static MotorOperatingPoint motor(const ElectricMotor &motor, const ElectricalMachineModel &scene);
};
