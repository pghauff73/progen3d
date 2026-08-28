#pragma once

#include <string>
#include <utility>
#include <vector>

enum class VegetationMeasuredCoordinateReferenceTransformIssueCode
{
	UnsupportedSchema,
	EmptyTransformIdentifier,
	EmptySourceCoordinateSystem,
	UnsupportedTargetCoordinateSystem,
	UnsupportedSourceCoordinateUnit,
	UnsupportedTargetCoordinateUnit,
	NonFiniteMatrix,
	NonAffineMatrix,
	SingularMatrix,
	EmptyUncertaintyStatement,
	EmptyEvidenceIdentifier,
	InvalidEvidenceSha256
};

class VegetationMeasuredCoordinateReferenceTransformIssue
{
public:
	VegetationMeasuredCoordinateReferenceTransformIssue(
		VegetationMeasuredCoordinateReferenceTransformIssueCode code,
		std::string message)
		: code_(code), message_(std::move(message))
	{
	}

	VegetationMeasuredCoordinateReferenceTransformIssueCode code() const
	{
		return code_;
	}
	const std::string &message() const { return message_; }

private:
	VegetationMeasuredCoordinateReferenceTransformIssueCode code_ =
		VegetationMeasuredCoordinateReferenceTransformIssueCode::UnsupportedSchema;
	std::string message_;
};

class VegetationMeasuredCoordinateReferenceTransformValidationReport
{
public:
	explicit VegetationMeasuredCoordinateReferenceTransformValidationReport(
		std::vector<VegetationMeasuredCoordinateReferenceTransformIssue> issues)
		: issues_(std::move(issues))
	{
	}

	bool valid() const { return issues_.empty(); }
	const std::vector<VegetationMeasuredCoordinateReferenceTransformIssue> &issues()
		const
	{
		return issues_;
	}

private:
	std::vector<VegetationMeasuredCoordinateReferenceTransformIssue> issues_;
};
