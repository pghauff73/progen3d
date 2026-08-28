#pragma once

#include <string>
#include <utility>
#include <vector>

enum class ModernCarSemanticValidationCode
{
	InvalidSourceCatalog,
	InvalidSemanticSections,
	InvalidWheelEnvelope,
	InvalidGenerationPolicy,
	NonfiniteField,
	NonfiniteMesh,
	NonWatertightMesh,
	InconsistentMeshOrientation,
	PackageBoundsMismatch,
	NondeterministicGeneration,
	MissingSemanticCurves,
	InvalidPanelGraph,
	DisconnectedBodyInWhite,
	OccupantContainmentFailure,
	FunctionalPackageFailure,
	SilhouetteThresholdFailure,
	SemanticImplicitCorrespondenceFailure,
	InvalidSurfaceDomain,
	SurfaceOwnershipFailure,
	GlassApertureFailure
};

class ModernCarSemanticValidationIssue
{
public:
	ModernCarSemanticValidationIssue(
		ModernCarSemanticValidationCode code,
		std::string message,
		std::vector<std::string> object_identifiers = {})
		: code_(code),
		  message_(std::move(message)),
		  object_identifiers_(std::move(object_identifiers))
	{
	}

	ModernCarSemanticValidationCode code() const { return code_; }
	const std::string &message() const { return message_; }
	const std::vector<std::string> &objectIdentifiers() const
	{
		return object_identifiers_;
	}

private:
	ModernCarSemanticValidationCode code_ =
		ModernCarSemanticValidationCode::InvalidSourceCatalog;
	std::string message_;
	std::vector<std::string> object_identifiers_;
};

class ModernCarSemanticValidationReport
{
public:
	void addIssue(ModernCarSemanticValidationIssue issue)
	{
		issues_.push_back(std::move(issue));
	}

	void append(const ModernCarSemanticValidationReport &other)
	{
		issues_.insert(issues_.end(), other.issues_.begin(), other.issues_.end());
	}

	bool isValid() const { return issues_.empty(); }
	const std::vector<ModernCarSemanticValidationIssue> &issues() const
	{
		return issues_;
	}

private:
	std::vector<ModernCarSemanticValidationIssue> issues_;
};
