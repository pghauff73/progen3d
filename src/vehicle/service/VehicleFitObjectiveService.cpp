#include "vehicle/service/VehicleFitObjectiveService.h"

#include <cmath>
#include <map>

float VehicleFitMeasurementSet::value(VehicleFitResidualTerm term) const
{
	switch (term) {
	case VehicleFitResidualTerm::Landmark: return landmark_;
	case VehicleFitResidualTerm::Silhouette: return silhouette_;
	case VehicleFitResidualTerm::CharacterCurve: return character_curve_;
	case VehicleFitResidualTerm::Package: return package_;
	case VehicleFitResidualTerm::GapClosure: return gap_closure_;
	case VehicleFitResidualTerm::Kinematic: return kinematic_;
	case VehicleFitResidualTerm::ClassA: return class_a_;
	case VehicleFitResidualTerm::ShapePrior: return shape_prior_;
	case VehicleFitResidualTerm::Complexity: return complexity_;
	}
	return 0.0f;
}

VehicleFitResidualReport VehicleFitObjectiveService::calculate(
	const VehicleFitObjective &objective,
	const VehicleFitMeasurementSet &measurements) const
{
	std::map<VehicleFitResidualTerm, float> weights;
	for (const VehicleFitTermWeight &weight : objective.termWeights()) {
		weights[weight.term()] = weight.weight();
	}
	const VehicleFitResidualTerm ordered_terms[] = {
		VehicleFitResidualTerm::Landmark,
		VehicleFitResidualTerm::Silhouette,
		VehicleFitResidualTerm::CharacterCurve,
		VehicleFitResidualTerm::Package,
		VehicleFitResidualTerm::GapClosure,
		VehicleFitResidualTerm::Kinematic,
		VehicleFitResidualTerm::ClassA,
		VehicleFitResidualTerm::ShapePrior,
		VehicleFitResidualTerm::Complexity};
	std::vector<VehicleFitResidualComponent> components;
	float total = 0.0f;
	for (VehicleFitResidualTerm term : ordered_terms) {
		const float raw = measurements.value(term);
		const float weight = weights.count(term) != 0u ? weights[term] : 0.0f;
		const float weighted = raw * weight;
		components.emplace_back(term, raw, weight, weighted);
		total += weighted;
	}
	std::vector<std::string> hard_failures;
	for (const VehicleFitConstraint &constraint : objective.constraints()) {
		const float residual = constraint.measuredResidual();
		if (constraint.strength() == VehicleConstraintStrength::Hard &&
		    (!std::isfinite(residual) || residual > constraint.tolerance())) {
			hard_failures.push_back(constraint.identifier());
		}
	}
	return VehicleFitResidualReport(
		std::move(components), std::move(hard_failures), total);
}
