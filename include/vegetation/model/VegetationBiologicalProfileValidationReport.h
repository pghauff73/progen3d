#pragma once

#include <string>
#include <utility>
#include <vector>

enum class VegetationBiologicalProfileValidationCode
{
	EmptyModelIdentifier,
	ArchitectureMismatch,
	EmptyArchitecturalArchetype,
	ContradictoryIdentityResolution,
	MissingScientificName,
	MissingCultivarName,
	MissingSpecimenIdentifier,
	MissingIdentityEvidence,
	DuplicateArchitecturalScale,
	MissingArchitecturalScale,
	DuplicateTopologyRelationship,
	MissingTopologyRelationship,
	MissingShootSystem,
	IncompleteShootTopologyCalibration,
	UnsupportedReferencePhenologyState,
	DuplicatePhenologyState,
	IncompleteRootCalibration,
	InvalidCanopyOpticalCalibration,
	InvalidBiomechanicalCalibration,
	IncompleteSemanticAnnotationCalibration,
	DuplicateSemanticAnnotation,
	InvalidFunctionalTraitCalibration,
	InvalidHydraulicCalibration,
	InvalidSizeAllometryCalibration,
	InvalidSubstrateRequirementCalibration,
	MissingEnvironmentalDriver,
	DuplicateEnvironmentalDriver,
	EmptyGeometryAuthority,
	EmptyBiologicalAuthority,
	EmptyUncertaintyStatement
};

class VegetationBiologicalProfileValidationIssue
{
public:
	VegetationBiologicalProfileValidationIssue(
		VegetationBiologicalProfileValidationCode code,
		std::string message)
		: code_(code), message_(std::move(message))
	{
	}

	VegetationBiologicalProfileValidationCode code() const { return code_; }
	const std::string &message() const { return message_; }

private:
	VegetationBiologicalProfileValidationCode code_ =
		VegetationBiologicalProfileValidationCode::EmptyModelIdentifier;
	std::string message_;
};

class VegetationBiologicalProfileValidationReport
{
public:
	void addIssue(
		VegetationBiologicalProfileValidationCode code,
		std::string message)
	{
		issues_.emplace_back(code, std::move(message));
	}

	bool passed() const { return issues_.empty(); }
	const std::vector<VegetationBiologicalProfileValidationIssue> &issues() const
	{
		return issues_;
	}

private:
	std::vector<VegetationBiologicalProfileValidationIssue> issues_;
};
