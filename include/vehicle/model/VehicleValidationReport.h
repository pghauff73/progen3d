#pragma once

#include <string>
#include <utility>
#include <vector>

enum class VehicleDiagnosticCode
{
	InvalidWheelbase,
	TireOutsideBody,
	InvalidBodySection,
	LoftFailure,
	GuideConflict,
	ShellOffsetFailure,
	InvalidPanelCut,
	PanelGapOverlap,
	ArchClearanceFailure,
	InvalidJoint,
	TravelCollision,
	InterfaceMismatch,
	PositioningFailure,
	DoorCollision,
	InvalidViewEvidence,
	InvalidSurfaceLandmark,
	InvalidCharacterCurveNetwork,
	InvalidSurfacePatchGraph,
	InvalidAperture,
	InvalidClosureAssembly,
	InvalidGuideRailJoint,
	InvalidClosureDependency,
	InvalidSweptVolume,
	InvalidEvidenceClassification,
	InvalidVehicleReferenceFrame,
	InvalidOccupantPackage,
	InvalidAutomotiveWireframe,
	InvalidClassASurfaceGraph,
	InvalidAutomotivePanelGap,
	InvalidBodySideAperture,
	InvalidHelicalGlassDrop,
	InvalidBodyInWhite,
	InvalidSuspensionHardpoint,
	InvalidWheelSweptEnvelope,
	InvalidTyreGeometry,
	InvalidRimGeometry,
	InvalidBrakeGeometry,
	InvalidAeroGeometry,
	InvalidVehicleFitObjective,
	InvalidParametricVariant,
	InvalidParametricSourceEvidence,
	InvalidParametricFitReport,
	InvalidMvp26Architecture
};

const char *vehicleDiagnosticCodeName(VehicleDiagnosticCode code);

class VehicleValidationIssue
{
public:
	VehicleValidationIssue(
		VehicleDiagnosticCode code,
		std::string message,
		std::vector<std::string> object_identifiers = {})
		: code_(code),
		  message_(std::move(message)),
		  object_identifiers_(std::move(object_identifiers))
	{
	}

	VehicleDiagnosticCode code() const { return code_; }
	const std::string &message() const { return message_; }
	const std::vector<std::string> &objectIdentifiers() const
	{
		return object_identifiers_;
	}

private:
	VehicleDiagnosticCode code_ = VehicleDiagnosticCode::InvalidWheelbase;
	std::string message_;
	std::vector<std::string> object_identifiers_;
};

class VehicleValidationReport
{
public:
	void addIssue(VehicleValidationIssue issue)
	{
		issues_.push_back(std::move(issue));
	}

	void append(const VehicleValidationReport &other)
	{
		issues_.insert(issues_.end(), other.issues_.begin(), other.issues_.end());
	}

	bool isValid() const { return issues_.empty(); }
	const std::vector<VehicleValidationIssue> &issues() const { return issues_; }

private:
	std::vector<VehicleValidationIssue> issues_;
};
