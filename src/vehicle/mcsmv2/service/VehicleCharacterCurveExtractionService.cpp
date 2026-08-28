#include "vehicle/mcsmv2/service/VehicleCharacterCurveExtractionService.h"

#include "vehicle/mcsmv2/model/VehicleSectionScalarField.h"
#include "vehicle/mcsmv2/service/VehicleSectionInterpolationRelationshipService.h"
#include "vehicle/mcsmv2/service/VehicleSemanticSectionFieldEvaluationService.h"

#include <array>
#include <stdexcept>
#include <vector>

namespace {

const std::array<VehicleCharacterCurveRole, 14> kCharacterCurveRoles{{
	VehicleCharacterCurveRole::CentreSpine,
	VehicleCharacterCurveRole::RoofCentre,
	VehicleCharacterCurveRole::RoofRailLeft,
	VehicleCharacterCurveRole::RoofRailRight,
	VehicleCharacterCurveRole::GlassShoulderLeft,
	VehicleCharacterCurveRole::GlassShoulderRight,
	VehicleCharacterCurveRole::BeltLeft,
	VehicleCharacterCurveRole::BeltRight,
	VehicleCharacterCurveRole::ShoulderLeft,
	VehicleCharacterCurveRole::ShoulderRight,
	VehicleCharacterCurveRole::RockerLeft,
	VehicleCharacterCurveRole::RockerRight,
	VehicleCharacterCurveRole::UnderbodyEdgeLeft,
	VehicleCharacterCurveRole::UnderbodyEdgeRight}};

glm::dvec3 pointFor(
	VehicleCharacterCurveRole role,
	double source_x,
	const VehicleSemanticSectionFieldEvaluationService &evaluation)
{
	switch (role) {
	case VehicleCharacterCurveRole::CentreSpine:
		return glm::dvec3(
			source_x,
			0.0,
			0.5 * (evaluation.evaluate(
				VehicleSectionScalarField::UnderbodyHeight, source_x) +
				evaluation.evaluate(
					VehicleSectionScalarField::RoofCrownHeight, source_x)));
	case VehicleCharacterCurveRole::RoofCentre:
		return glm::dvec3(
			source_x, 0.0,
			evaluation.evaluate(VehicleSectionScalarField::RoofCrownHeight, source_x));
	case VehicleCharacterCurveRole::RoofRailLeft:
	case VehicleCharacterCurveRole::RoofRailRight: {
		const double sign = role == VehicleCharacterCurveRole::RoofRailLeft ? -1.0 : 1.0;
		return glm::dvec3(
			source_x,
			sign * evaluation.evaluate(
				VehicleSectionScalarField::RoofRailHalfWidth, source_x),
			evaluation.evaluate(VehicleSectionScalarField::RoofRailHeight, source_x));
	}
	case VehicleCharacterCurveRole::GlassShoulderLeft:
	case VehicleCharacterCurveRole::GlassShoulderRight: {
		const double sign =
			role == VehicleCharacterCurveRole::GlassShoulderLeft ? -1.0 : 1.0;
		return glm::dvec3(
			source_x,
			sign * evaluation.evaluate(
				VehicleSectionScalarField::GlassShoulderHalfWidth, source_x),
			evaluation.evaluate(
				VehicleSectionScalarField::GlassShoulderHeight, source_x));
	}
	case VehicleCharacterCurveRole::BeltLeft:
	case VehicleCharacterCurveRole::BeltRight: {
		const double sign = role == VehicleCharacterCurveRole::BeltLeft ? -1.0 : 1.0;
		return glm::dvec3(
			source_x,
			sign * evaluation.evaluate(
				VehicleSectionScalarField::BeltHalfWidth, source_x),
			evaluation.evaluate(VehicleSectionScalarField::BeltHeight, source_x));
	}
	case VehicleCharacterCurveRole::ShoulderLeft:
	case VehicleCharacterCurveRole::ShoulderRight: {
		const double sign = role == VehicleCharacterCurveRole::ShoulderLeft ? -1.0 : 1.0;
		return glm::dvec3(
			source_x,
			sign * evaluation.evaluate(
				VehicleSectionScalarField::ShoulderHalfWidth, source_x),
			evaluation.evaluate(VehicleSectionScalarField::ShoulderHeight, source_x));
	}
	case VehicleCharacterCurveRole::RockerLeft:
	case VehicleCharacterCurveRole::RockerRight: {
		const double sign = role == VehicleCharacterCurveRole::RockerLeft ? -1.0 : 1.0;
		return glm::dvec3(
			source_x,
			sign * evaluation.evaluate(
				VehicleSectionScalarField::RockerHalfWidth, source_x),
			evaluation.evaluate(VehicleSectionScalarField::RockerHeight, source_x));
	}
	case VehicleCharacterCurveRole::UnderbodyEdgeLeft:
	case VehicleCharacterCurveRole::UnderbodyEdgeRight: {
		const double sign =
			role == VehicleCharacterCurveRole::UnderbodyEdgeLeft ? -1.0 : 1.0;
		return glm::dvec3(
			source_x,
			sign * evaluation.evaluate(
				VehicleSectionScalarField::UnderbodyHalfWidth, source_x),
			evaluation.evaluate(
				VehicleSectionScalarField::UnderbodyHeight, source_x));
	}
	}
	throw std::logic_error("Unsupported vehicle character curve role.");
}

} // namespace

VehicleCharacterCurveNetwork VehicleCharacterCurveExtractionService::extract(
	const ModernCarSemanticVariant &variant,
	std::size_t samples_per_curve) const
{
	if (samples_per_curve < 2u) {
		throw std::invalid_argument(
			"Character curve extraction requires at least two samples per curve.");
	}
	const VehicleSemanticSectionField &section_field = variant.sectionField();
	VehicleSemanticSectionFieldEvaluationService evaluation(section_field);
	VehicleSectionInterpolationRelationshipService relationship_service;
	std::vector<VehicleCharacterCurve> curves;
	curves.reserve(kCharacterCurveRoles.size());
	for (VehicleCharacterCurveRole role : kCharacterCurveRoles) {
		std::vector<glm::dvec3> points;
		std::vector<VehicleSectionInterpolationRelationship> relationships;
		points.reserve(samples_per_curve);
		relationships.reserve(samples_per_curve);
		for (std::size_t sample = 0u; sample < samples_per_curve; ++sample) {
			const double parameter = static_cast<double>(sample) /
				static_cast<double>(samples_per_curve - 1u);
			const double source_x = evaluation.rearSourceX() +
				(evaluation.frontSourceX() - evaluation.rearSourceX()) * parameter;
			points.push_back(pointFor(role, source_x, evaluation));
			relationships.push_back(
				relationship_service.resolve(section_field, source_x));
		}
		curves.emplace_back(
			vehicleCharacterCurveRoleName(role), role,
			std::move(points), std::move(relationships));
	}
	return VehicleCharacterCurveNetwork(
		variant.identifier() + ".CharacterCurveNetwork", std::move(curves));
}
