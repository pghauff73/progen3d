#pragma once

#include "vehicle/model/VehicleMvp25Evidence.h"

#include <string>
#include <utility>
#include <vector>

enum class VehicleFitResidualTerm
{
	Landmark,
	Silhouette,
	CharacterCurve,
	Package,
	GapClosure,
	Kinematic,
	ClassA,
	ShapePrior,
	Complexity
};

class VehicleFitTermWeight
{
public:
	VehicleFitTermWeight(VehicleFitResidualTerm term, float weight)
		: term_(term), weight_(weight)
	{
	}

	VehicleFitResidualTerm term() const { return term_; }
	float weight() const { return weight_; }

private:
	VehicleFitResidualTerm term_ = VehicleFitResidualTerm::Landmark;
	float weight_ = 0.0f;
};

class VehicleFitConstraint
{
public:
	VehicleFitConstraint(
		std::string identifier,
		VehicleFitResidualTerm term,
		VehicleConstraintStrength strength,
		float tolerance,
		float measured_residual)
		: identifier_(std::move(identifier)),
		  term_(term),
		  strength_(strength),
		  tolerance_(tolerance),
		  measured_residual_(measured_residual)
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleFitResidualTerm term() const { return term_; }
	VehicleConstraintStrength strength() const { return strength_; }
	float tolerance() const { return tolerance_; }
	float measuredResidual() const { return measured_residual_; }

private:
	std::string identifier_;
	VehicleFitResidualTerm term_ = VehicleFitResidualTerm::Landmark;
	VehicleConstraintStrength strength_ = VehicleConstraintStrength::Soft;
	float tolerance_ = 0.0f;
	float measured_residual_ = 0.0f;
};

class VehicleFitResidualComponent
{
public:
	VehicleFitResidualComponent(
		VehicleFitResidualTerm term,
		float raw_error,
		float weight,
		float weighted_error)
		: term_(term),
		  raw_error_(raw_error),
		  weight_(weight),
		  weighted_error_(weighted_error)
	{
	}

	VehicleFitResidualTerm term() const { return term_; }
	float rawError() const { return raw_error_; }
	float weight() const { return weight_; }
	float weightedError() const { return weighted_error_; }

private:
	VehicleFitResidualTerm term_ = VehicleFitResidualTerm::Landmark;
	float raw_error_ = 0.0f;
	float weight_ = 0.0f;
	float weighted_error_ = 0.0f;
};

class VehicleFitObjective
{
public:
	VehicleFitObjective(
		std::vector<VehicleFitTermWeight> term_weights,
		std::vector<VehicleFitConstraint> constraints,
		float patch_count_penalty,
		float control_point_penalty,
		float span_penalty)
		: term_weights_(std::move(term_weights)),
		  constraints_(std::move(constraints)),
		  patch_count_penalty_(patch_count_penalty),
		  control_point_penalty_(control_point_penalty),
		  span_penalty_(span_penalty)
	{
	}

	const std::vector<VehicleFitTermWeight> &termWeights() const { return term_weights_; }
	const std::vector<VehicleFitConstraint> &constraints() const { return constraints_; }
	float patchCountPenalty() const { return patch_count_penalty_; }
	float controlPointPenalty() const { return control_point_penalty_; }
	float spanPenalty() const { return span_penalty_; }

private:
	std::vector<VehicleFitTermWeight> term_weights_;
	std::vector<VehicleFitConstraint> constraints_;
	float patch_count_penalty_ = 0.0f;
	float control_point_penalty_ = 0.0f;
	float span_penalty_ = 0.0f;
};

class VehicleFitResidualReport
{
public:
	VehicleFitResidualReport(
		std::vector<VehicleFitResidualComponent> components,
		std::vector<std::string> hard_failure_identifiers,
		float total_weighted_error)
		: components_(std::move(components)),
		  hard_failure_identifiers_(std::move(hard_failure_identifiers)),
		  total_weighted_error_(total_weighted_error)
	{
	}

	const std::vector<VehicleFitResidualComponent> &components() const
	{
		return components_;
	}
	const std::vector<std::string> &hardFailureIdentifiers() const
	{
		return hard_failure_identifiers_;
	}
	float totalWeightedError() const { return total_weighted_error_; }
	bool isValid() const { return hard_failure_identifiers_.empty(); }

private:
	std::vector<VehicleFitResidualComponent> components_;
	std::vector<std::string> hard_failure_identifiers_;
	float total_weighted_error_ = 0.0f;
};
