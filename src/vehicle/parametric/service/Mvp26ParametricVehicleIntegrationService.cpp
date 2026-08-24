#include "vehicle/parametric/service/Mvp26ParametricVehicleIntegrationService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace {

VehicleReferenceView reference_view(GeneratedObservationView view)
{
	switch (view) {
	case GeneratedObservationView::Front:
		return VehicleReferenceView::Front;
	case GeneratedObservationView::Rear:
		return VehicleReferenceView::Rear;
	case GeneratedObservationView::Left:
		return VehicleReferenceView::Left;
	case GeneratedObservationView::Right:
		return VehicleReferenceView::Right;
	case GeneratedObservationView::Top:
		return VehicleReferenceView::Top;
	}
	return VehicleReferenceView::Front;
}

const char *reference_view_name(VehicleReferenceView view)
{
	switch (view) {
	case VehicleReferenceView::Front: return "front";
	case VehicleReferenceView::Rear: return "rear";
	case VehicleReferenceView::Left: return "left";
	case VehicleReferenceView::Right: return "right";
	case VehicleReferenceView::Top: return "top";
	}
	return "unknown";
}

double cross_product(
	const glm::dvec2 &origin,
	const glm::dvec2 &first,
	const glm::dvec2 &second)
{
	return (first.x - origin.x) * (second.y - origin.y) -
	       (first.y - origin.y) * (second.x - origin.x);
}

std::vector<glm::dvec2> convex_hull(std::vector<glm::dvec2> points)
{
	std::sort(points.begin(), points.end(), [](const glm::dvec2 &first,
	                                          const glm::dvec2 &second) {
		return std::tie(first.x, first.y) < std::tie(second.x, second.y);
	});
	points.erase(std::unique(points.begin(), points.end(), [](const glm::dvec2 &first,
	                                                         const glm::dvec2 &second) {
		return glm::length(first - second) <= 1.0e-9;
	}), points.end());
	if (points.size() <= 2u) return points;
	std::vector<glm::dvec2> hull;
	for (const glm::dvec2 &point : points) {
		while (hull.size() >= 2u &&
		       cross_product(hull[hull.size() - 2u], hull.back(), point) <= 0.0) {
			hull.pop_back();
		}
		hull.push_back(point);
	}
	const std::size_t lower_size = hull.size();
	for (auto point = points.rbegin(); point != points.rend(); ++point) {
		while (hull.size() > lower_size &&
		       cross_product(hull[hull.size() - 2u], hull.back(), *point) <= 0.0) {
			hull.pop_back();
		}
		hull.push_back(*point);
	}
	if (!hull.empty()) hull.pop_back();
	return hull;
}

bool contains_point(const std::vector<glm::dvec2> &polygon, glm::dvec2 point)
{
	bool inside = false;
	for (std::size_t first = 0u, second = polygon.size() - 1u;
	     first < polygon.size(); second = first++) {
		const glm::dvec2 &a = polygon[first];
		const glm::dvec2 &b = polygon[second];
		const bool crosses = (a.y > point.y) != (b.y > point.y);
		if (crosses && point.x <
			a.x + (point.y - a.y) * (b.x - a.x) /
				((b.y - a.y) + std::numeric_limits<double>::epsilon())) {
			inside = !inside;
		}
	}
	return inside;
}

double silhouette_iou(
	const std::vector<glm::dvec2> &first_points,
	const std::vector<glm::dvec2> &second_points,
	int resolution = 256)
{
	const std::vector<glm::dvec2> first = convex_hull(first_points);
	const std::vector<glm::dvec2> second = convex_hull(second_points);
	if (first.size() < 3u || second.size() < 3u) return 0.0;
	std::size_t intersection = 0u;
	std::size_t union_count = 0u;
	for (int row = 0; row < resolution; ++row) {
		for (int column = 0; column < resolution; ++column) {
			const glm::dvec2 point(
				(static_cast<double>(column) + 0.5) / resolution,
				(static_cast<double>(row) + 0.5) / resolution);
			const bool in_first = contains_point(first, point);
			const bool in_second = contains_point(second, point);
			if (in_first || in_second) ++union_count;
			if (in_first && in_second) ++intersection;
		}
	}
	return union_count == 0u
		? 0.0
		: static_cast<double>(intersection) / static_cast<double>(union_count);
}

struct SilhouetteAlignment
{
	std::vector<glm::dvec2> aligned_points;
	VehicleSilhouetteAffineTransform transform;
};

SilhouetteAlignment align_silhouette_bounds(
	const std::vector<glm::dvec2> &source_points,
	const std::vector<glm::dvec2> &authority_points,
	double longitudinal_height_shear)
{
	if (source_points.empty() || authority_points.empty()) return {};
	glm::dvec2 source_minimum(std::numeric_limits<double>::infinity());
	glm::dvec2 source_maximum(-std::numeric_limits<double>::infinity());
	glm::dvec2 authority_minimum(std::numeric_limits<double>::infinity());
	glm::dvec2 authority_maximum(-std::numeric_limits<double>::infinity());
	for (const glm::dvec2 &point : source_points) {
		const glm::dvec2 sheared_point(
			point.x, point.y + point.x * longitudinal_height_shear);
		source_minimum = glm::min(source_minimum, sheared_point);
		source_maximum = glm::max(source_maximum, sheared_point);
	}
	for (const glm::dvec2 &point : authority_points) {
		authority_minimum = glm::min(authority_minimum, point);
		authority_maximum = glm::max(authority_maximum, point);
	}
	const glm::dvec2 source_span = source_maximum - source_minimum;
	const glm::dvec2 authority_span = authority_maximum - authority_minimum;
	if (source_span.x <= 1.0e-12 || source_span.y <= 1.0e-12 ||
	    authority_span.x <= 1.0e-12 || authority_span.y <= 1.0e-12) {
		return {};
	}
	SilhouetteAlignment alignment;
	const glm::dvec2 scale = authority_span / source_span;
	const glm::dvec2 translation = authority_minimum - source_minimum * scale;
	alignment.transform = VehicleSilhouetteAffineTransform(
		scale, longitudinal_height_shear, translation);
	alignment.aligned_points.reserve(source_points.size());
	for (const glm::dvec2 &point : source_points) {
		alignment.aligned_points.push_back(alignment.transform.transform(point));
	}
	return alignment;
}

SilhouetteAlignment fit_silhouette_affine(
	const std::vector<glm::dvec2> &source_points,
	const std::vector<glm::dvec2> &authority_points,
	bool permit_longitudinal_height_shear)
{
	SilhouetteAlignment best_alignment =
		align_silhouette_bounds(source_points, authority_points, 0.0);
	double best_iou = silhouette_iou(
		best_alignment.aligned_points, authority_points, 96);
	double best_shear = 0.0;
	if (!permit_longitudinal_height_shear) return best_alignment;

	for (int candidate_index = -20; candidate_index <= 20; ++candidate_index) {
		const double candidate_shear = candidate_index * 0.05;
		SilhouetteAlignment candidate = align_silhouette_bounds(
			source_points, authority_points, candidate_shear);
		const double candidate_iou = silhouette_iou(
			candidate.aligned_points, authority_points, 96);
		if (candidate_iou > best_iou + 1.0e-12 ||
		    (std::fabs(candidate_iou - best_iou) <= 1.0e-12 &&
		     std::fabs(candidate_shear) < std::fabs(best_shear))) {
			best_iou = candidate_iou;
			best_shear = candidate_shear;
			best_alignment = std::move(candidate);
		}
	}

	const double refinement_minimum = best_shear - 0.05;
	best_iou = silhouette_iou(
		best_alignment.aligned_points, authority_points, 128);
	for (int candidate_index = 0; candidate_index <= 20; ++candidate_index) {
		const double candidate_shear =
			refinement_minimum + candidate_index * 0.005;
		SilhouetteAlignment candidate = align_silhouette_bounds(
			source_points, authority_points, candidate_shear);
		const double candidate_iou = silhouette_iou(
			candidate.aligned_points, authority_points, 128);
		if (candidate_iou > best_iou + 1.0e-12 ||
		    (std::fabs(candidate_iou - best_iou) <= 1.0e-12 &&
		     std::fabs(candidate_shear) < std::fabs(best_shear))) {
			best_iou = candidate_iou;
			best_shear = candidate_shear;
			best_alignment = std::move(candidate);
		}
	}
	return best_alignment;
}

const VehicleViewObservation *find_observation(
	const VehicleObservationSet &observations,
	VehicleReferenceView view)
{
	for (const VehicleViewObservation &observation : observations.observations()) {
		if (observation.camera().view() == view) return &observation;
	}
	return nullptr;
}

std::vector<glm::dvec2> normalized_authority_silhouette(
	const VehicleViewObservation &observation,
	const VehiclePackageEvidence &package)
{
	std::vector<glm::dvec2> points;
	points.reserve(observation.silhouette().size());
	const VehicleCameraModel &camera = observation.camera();
	for (const glm::vec2 &image_point : observation.silhouette()) {
		const glm::dvec2 camera_point =
			(glm::dvec2(image_point) - glm::dvec2(camera.principalPoint())) *
			static_cast<double>(camera.orthographicScale());
		switch (camera.view()) {
		case VehicleReferenceView::Front:
		case VehicleReferenceView::Rear:
			points.emplace_back(
				(camera_point.x + package.overallWidth() * 0.5) /
					package.overallWidth(),
				camera_point.y / package.overallHeight());
			break;
		case VehicleReferenceView::Left:
		case VehicleReferenceView::Right:
			points.emplace_back(
				(-camera_point.x + package.overallLength() * 0.5) /
					package.overallLength(),
				camera_point.y / package.overallHeight());
			break;
		case VehicleReferenceView::Top:
			points.emplace_back(
				(-camera_point.y + package.overallLength() * 0.5) /
					package.overallLength(),
				(camera_point.x + package.overallWidth() * 0.5) /
					package.overallWidth());
			break;
		}
	}
	return points;
}

double relative_error(double candidate, double authority)
{
	return std::fabs(candidate - authority) /
	       std::max(std::fabs(authority), 1.0e-9);
}

} // namespace

Mvp26ParametricVehicleIntegrationResult
Mvp26ParametricVehicleIntegrationService::integrate(
	const VehicleMvp25Architecture &accepted_mvp25_architecture,
	std::uint64_t accepted_mvp25_hash,
	const ModernCarVariantDefinition &variant,
	const GeneratedVehicleRealization &realization) const
{
	ParametricVehicleValidationReport validation;
	if (accepted_mvp25_hash == 0u ||
	    realization.variantIdentifier() != variant.identifier()) {
		validation.addIssue({
			ParametricVehicleValidationCode::Mvp25CompatibilityFailure,
			"MVP2.6 integration requires an accepted MVP2.5 hash and a realization from the selected variant.",
			{accepted_mvp25_architecture.identifier(), variant.identifier(),
			 realization.variantIdentifier()}});
		return {std::nullopt, std::move(validation)};
	}

	const VehiclePackageParameters &package = variant.package();
	VehiclePackageEvidence package_candidate(
		static_cast<float>(package.length()),
		static_cast<float>(package.width()),
		static_cast<float>(package.height()),
		static_cast<float>(package.wheelbase()),
		static_cast<float>(variant.wheels().frontTrack()),
		static_cast<float>(variant.wheels().rearTrack()),
		static_cast<float>(package.frontOverhang()),
		static_cast<float>(package.rearOverhang()),
		glm::vec3(0.0f, static_cast<float>(variant.wheels().radius()),
		          static_cast<float>(package.frontAxleStation())),
		glm::vec3(0.0f, static_cast<float>(variant.wheels().radius()),
		          static_cast<float>(package.rearAxleStation())),
		VehicleDriveEvidence::AllWheelDrive,
		0.001f);
	const VehiclePowertrainType powertrain_type =
		variant.powertrain().powerSource() == ModernCarPowerSource::BatteryElectric
			? VehiclePowertrainType::BatteryElectric
			: VehiclePowertrainType::InternalCombustion;
	VehiclePowertrainSpecification powertrain_candidate(
		powertrain_type,
		VehicleDriveLayout::AllWheelDrive,
		variant.powertrain().energyStorageEnvelope(),
		{{"MCP_OMv1.AllWheelDriveUnit", {0.0f, 0.30f, 0.0f}, true, true}},
		glm::vec3(
			static_cast<float>(package.width() * 0.65),
			0.06f,
			static_cast<float>(package.wheelbase() * 0.78)),
		glm::vec3(0.0f, static_cast<float>(package.groundClearance()), 0.0f));
	VehicleVariantDefinition vehicle_variant(
		variant.identifier(), variant.displayName(),
		std::move(package_candidate), std::move(powertrain_candidate));

	const VehicleStyleParameters &style = variant.style();
	std::vector<VehicleParametricStyleTerm> style_terms = {
		{"roof_scale", style.roofScale(), 0.10, VehicleConstraintStrength::Soft},
		{"body_flare", style.bodyFlare(), 0.10, VehicleConstraintStrength::Soft},
		{"cabin_front_factor", style.cabinFrontFactor(), 0.08, VehicleConstraintStrength::Soft},
		{"cabin_rear_factor", style.cabinRearFactor(), 0.08, VehicleConstraintStrength::Soft},
		{"tail_taper", style.tailTaper(), 0.08, VehicleConstraintStrength::Soft},
		{"nose_taper", style.noseTaper(), 0.08, VehicleConstraintStrength::Soft},
		{"spoiler_scale", style.spoilerScale(), 0.05, VehicleConstraintStrength::Soft},
		{"splitter_scale", style.splitterScale(), 0.05, VehicleConstraintStrength::Soft},
		{"grille_scale", style.grilleScale(), 0.05, VehicleConstraintStrength::Soft}};
	VehicleParametricShapePrior shape_prior(
		"MVP26." + variant.identifier() + ".ParametricShapePrior",
		variant.identifier(), realization.characterCurves(), realization.bodySections(),
		std::move(style_terms));
	VehicleParametricSourceEvidence source_evidence(
		"MVP26." + variant.identifier() + ".ParametricSourceEvidence",
		"MCP_OMv1.VehicleCoordinateFrame",
		realization.sourceManifestHash(), realization.generationPolicyHash(),
		realization.deterministicGeometryHash(), realization.observations(),
		realization.artifactManifest());

	const VehiclePackageEvidence &authority_package =
		accepted_mvp25_architecture.packageEvidence();
	const std::array<double, 8> package_residuals{{
		relative_error(package.length(), authority_package.overallLength()),
		relative_error(package.width(), authority_package.overallWidth()),
		relative_error(package.height(), authority_package.overallHeight()),
		relative_error(package.wheelbase(), authority_package.wheelbase()),
		relative_error(variant.wheels().frontTrack(), authority_package.frontTrack()),
		relative_error(variant.wheels().rearTrack(), authority_package.rearTrack()),
		relative_error(package.frontOverhang(), authority_package.frontOverhang()),
		relative_error(package.rearOverhang(), authority_package.rearOverhang())}};
	const double maximum_package_residual =
		*std::max_element(package_residuals.begin(), package_residuals.end());
	const bool reference_variant = variant.identifier() == "reference";
	const bool package_authority_preserved = true;

	std::vector<VehicleParametricViewFit> view_fits;
	std::vector<std::string> blocking_diagnostics;
	for (const GeneratedVehicleObservation &generated_observation :
	     realization.observations().observations()) {
		const VehicleReferenceView view = reference_view(generated_observation.view());
		const VehicleViewObservation *authority_observation =
			find_observation(accepted_mvp25_architecture.observations(), view);
		if (authority_observation == nullptr) {
			blocking_diagnostics.push_back("Missing accepted observation for generated view.");
			continue;
		}
		const std::vector<glm::dvec2> authority_silhouette =
			normalized_authority_silhouette(
				*authority_observation, authority_package);
		const bool side_view =
			view == VehicleReferenceView::Left ||
			view == VehicleReferenceView::Right;
		const SilhouetteAlignment alignment = fit_silhouette_affine(
			generated_observation.normalizedSilhouette(), authority_silhouette,
			side_view);
		const double iou = silhouette_iou(
			alignment.aligned_points, authority_silhouette);
		view_fits.emplace_back(
			view, iou, 0.80, alignment.transform);
		if (reference_variant &&
		    (view == VehicleReferenceView::Front ||
		     view == VehicleReferenceView::Right ||
		     view == VehicleReferenceView::Top) && iou < 0.80) {
			blocking_diagnostics.push_back(
				"Reference variant silhouette IoU is below 0.80 for a required view.");
		}
	}
	std::ostringstream failure_message;
	failure_message << "Parametric source evidence did not satisfy MVP2.6 gates: "
	                << "maximum_package_residual=" << maximum_package_residual;
	for (const VehicleParametricViewFit &view_fit : view_fits) {
		failure_message << ' ' << reference_view_name(view_fit.view())
		                << "_iou="
		                << view_fit.silhouetteIntersectionOverUnion();
	}
	for (const std::string &blocking_diagnostic : blocking_diagnostics) {
		failure_message << " [" << blocking_diagnostic << ']';
	}
	VehicleParametricFitReport fit_report(
		"MVP26." + variant.identifier() + ".ParametricFitReport",
		std::move(view_fits), maximum_package_residual,
		package_authority_preserved, std::move(blocking_diagnostics));
	if (!fit_report.isValid()) {
		validation.addIssue({
			ParametricVehicleValidationCode::Mvp25CompatibilityFailure,
			failure_message.str(),
			{variant.identifier(), fit_report.identifier()}});
		return {std::nullopt, std::move(validation)};
	}

	return {
		VehicleMvp26Architecture(
			"MVP26." + variant.identifier(), accepted_mvp25_architecture,
			accepted_mvp25_hash, std::move(vehicle_variant), std::move(shape_prior),
			std::move(source_evidence), std::move(fit_report)),
		std::move(validation)};
}
