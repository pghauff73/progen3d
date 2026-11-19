#pragma once

// Inputs at one instant. Body mass excludes the separately modelled motor rotor.
struct CoaxialDriveStage {
	double hoopMassKg, effectiveRadiusM;
	double relativeDegreesPerS, relativeDegreesPerS2;
	double signedSpeedRatio, drivingEfficiency, backdriveEfficiency;
	double rotorInertiaKgM2, runningDragNm;
};

struct CoaxialDriveResponse {
	double hoopInertiaKgM2, relativeRadPerS, worldRadPerS, worldRadPerS2;
	double motorRpm, rotorWorldRadPerS, rotorWorldRadPerS2;
	double hoopJointTorqueNm, gearboxOutputTorqueNm, motorTorqueNm, motorPowerW;
	double hoopEnergyJ, chainHoopEnergyJ, chainRotorEnergyJ;
};
