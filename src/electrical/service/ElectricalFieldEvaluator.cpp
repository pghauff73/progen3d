#include "electrical/service/ElectricalFieldEvaluator.h"

#include <cmath>

namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double permeabilityOfAir = 1.25663706127e-6;
constexpr double permittivityOfAir = 8.8541878188e-12;

bool beyondLinearRange(double fluxDensity, const CoreMaterial &material) {
    return material.fluxDensityLimit && std::abs(fluxDensity) >= *material.fluxDensityLimit;
}
}

WindingFieldSample ElectricalFieldEvaluator::winding(const FieldCoil &coil, const MagnetWire &wire) {
    WindingFieldSample sample{};
    sample.wireLength = coil.turns * coil.meanTurnLength;
    sample.resistance = wire.resistancePerMetre() * sample.wireLength;
    sample.currentDensity = coil.current / wire.conductorArea();
    sample.conductorField = wire.conductor->resistivityAt(wire.temperature) * sample.currentDensity;
    sample.ohmicPower = coil.current * coil.current * sample.resistance;
    sample.insulationField = coil.insulationVoltage / wire.insulationThickness;
    sample.displacement = permittivityOfAir * wire.insulation->relativePermittivity * sample.insulationField;
    const double effectiveLength = coil.coreLength / coil.core->relativePermeability + coil.gap;
    sample.fluxDensity = permeabilityOfAir * coil.turns * coil.current / effectiveLength;
    sample.gapField = sample.fluxDensity / permeabilityOfAir;
    sample.coreField = sample.gapField / coil.core->relativePermeability;
    sample.flux = coil.area * sample.fluxDensity;
    sample.inductance = permeabilityOfAir * coil.turns * coil.turns * coil.area / effectiveLength;
    sample.storedEnergy = 0.5 * sample.inductance * coil.current * coil.current;
    sample.beyondCoreLimit = beyondLinearRange(sample.fluxDensity, *coil.core);
    return sample;
}

MagnetFieldSample ElectricalFieldEvaluator::magnet(const PermanentMagnet &magnet) {
    const double remanence = magnet.polarity * magnet.material->remanence;
    const double recoil = magnet.material->recoilPermeability;
    const double equivalentGap = magnet.gap + magnet.returnLength / magnet.returnCore->relativePermeability;
    const double fluxDensity = remanence * magnet.length / (magnet.length + recoil * equivalentGap);
    return {fluxDensity, (fluxDensity - remanence) / (permeabilityOfAir * recoil),
            fluxDensity / permeabilityOfAir, fluxDensity * magnet.area,
            magnet.returnLength > 0 && beyondLinearRange(fluxDensity, *magnet.returnCore)};
}

MotorOperatingPoint ElectricalFieldEvaluator::motor(const ElectricMotor &motor, const ElectricalMachineModel &scene) {
    const auto &coil = scene.coils.at(motor.windingId);
    const auto phase = winding(coil, scene.wires.at(coil.wireId));
    const double omega = motor.rpm * 2 * pi / 60;
    MotorOperatingPoint point{};
    const bool dc = motor.topology == MotorTopology::BrushedDC;
    const bool sixStep = motor.topology == MotorTopology::SixStepOutrunner;
    double phaseRms = std::abs(coil.current);
    if (sixStep) phaseRms *= std::sqrt(2.0 / 3.0);
    else if (!dc) phaseRms /= std::sqrt(2.0);
    point.lineCurrent = phaseRms * (motor.connection == WindingConnection::Delta ? std::sqrt(3.0) : 1.0);
    point.windingLoss = (dc ? 1 : 3) * phaseRms * phaseRms * phase.resistance;
    point.beyondCoreLimit = phase.beyondCoreLimit;
    if (motor.cage) {
        const auto &cage = *motor.cage;
        point.electricalFrequency = cage.supplyFrequency;
        point.synchronousRpm = 60 * cage.supplyFrequency / motor.polePairs;
        const double slip = 1 - motor.rpm / *point.synchronousRpm;
        point.slip = slip;
        point.rotorFrequency = std::abs(slip) * cage.supplyFrequency;
        const double resistance = cage.conductor->resistivityAt(cage.temperature) * cage.pathLength / cage.barArea;
        const double reactance = 2 * pi * cage.supplyFrequency * cage.leakageInductance;
        const double impedance = std::hypot(resistance, slip * reactance);
        const double currentScale = cage.standstillEmfRms / impedance;
        const double powerScale = cage.bars * currentScale * currentScale * resistance;
        point.rotorLoss = slip * slip * powerScale;
        point.torque = slip * powerScale / (2 * pi * cage.supplyFrequency / motor.polePairs);
    } else {
        const auto field = magnet(scene.magnets.at(motor.magnetId));
        point.beyondCoreLimit = point.beyondCoreLimit || field.beyondCoreLimit;
        point.electricalFrequency = std::abs(motor.rpm) * motor.polePairs / 60;
        const double linkage = coil.turns * motor.windingFactor * field.flux;
        double emfConstant = motor.polePairs * linkage;
        if (dc) emfConstant *= 2 / pi;
        // The integral of the unit 120-degree flat-top trapezoid over one
        // positive half-wave is 5*pi/6. Linkage traverses twice its peak.
        if (sixStep) emfConstant *= 12 / (5 * pi);
        if (sixStep) {
            point.torque = 2 * emfConstant * coil.current;
            point.lineBackEmf = 2 * emfConstant * omega; // signed voltage of the active pair
        } else if (dc) {
            point.torque = emfConstant * coil.current;
            point.lineBackEmf = emfConstant * omega;
        }
        else {
            point.torque = 1.5 * emfConstant * coil.current;
            const double phaseEmfRms = std::abs(emfConstant * omega) / std::sqrt(2.0);
            point.lineBackEmf = phaseEmfRms * (motor.connection == WindingConnection::Wye ? std::sqrt(3.0) : 1);
        }
    }
    point.convertedPower = point.torque * omega;
    return point;
}
