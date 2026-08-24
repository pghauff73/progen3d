#include "vehicle/parametric/model/ParametricVehicleValidationReport.h"

const char *parametricVehicleValidationCodeName(
	ParametricVehicleValidationCode code)
{
	switch (code) {
	case ParametricVehicleValidationCode::InvalidCoordinateFrame:
		return "InvalidCoordinateFrame";
	case ParametricVehicleValidationCode::InvalidPackage:
		return "InvalidPackage";
	case ParametricVehicleValidationCode::InvalidWheelParameters:
		return "InvalidWheelParameters";
	case ParametricVehicleValidationCode::InvalidStationFunction:
		return "InvalidStationFunction";
	case ParametricVehicleValidationCode::InvalidPowertrainIntent:
		return "InvalidPowertrainIntent";
	case ParametricVehicleValidationCode::InvalidFieldDefinition:
		return "InvalidFieldDefinition";
	case ParametricVehicleValidationCode::InvalidGenerationPolicy:
		return "InvalidGenerationPolicy";
	case ParametricVehicleValidationCode::NonfiniteGeneratedMesh:
		return "NonfiniteGeneratedMesh";
	case ParametricVehicleValidationCode::PackageBoundsMismatch:
		return "PackageBoundsMismatch";
	case ParametricVehicleValidationCode::NonWatertightGeneratedMesh:
		return "NonWatertightGeneratedMesh";
	case ParametricVehicleValidationCode::MissingCharacterCurves:
		return "MissingCharacterCurves";
	case ParametricVehicleValidationCode::MissingBodySections:
		return "MissingBodySections";
	case ParametricVehicleValidationCode::MissingObservations:
		return "MissingObservations";
	case ParametricVehicleValidationCode::MissingProvenance:
		return "MissingProvenance";
	case ParametricVehicleValidationCode::NondeterministicGeneration:
		return "NondeterministicGeneration";
	case ParametricVehicleValidationCode::SilhouetteThresholdFailure:
		return "SilhouetteThresholdFailure";
	case ParametricVehicleValidationCode::Mvp25CompatibilityFailure:
		return "Mvp25CompatibilityFailure";
	}
	return "UnknownParametricVehicleValidationCode";
}
