#include "vehicle/parametric/service/ModernCarParametricValidationService.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>

namespace {

bool finite_positive(double value)
{
	return std::isfinite(value) && value > 0.0;
}

bool finite_nonnegative(double value)
{
	return std::isfinite(value) && value >= 0.0;
}

void validate_station_function(
	const StationFunction &function,
	ParametricVehicleValidationReport *report,
	const std::string &variant_identifier)
{
	bool valid = !function.identifier().empty() && function.knots().size() >= 2u;
	double previous_position = -1.0;
	for (const StationFunctionKnot &knot : function.knots()) {
		valid = valid && std::isfinite(knot.position()) && std::isfinite(knot.value()) &&
		        knot.position() >= 0.0 && knot.position() <= 1.0 &&
		        knot.position() > previous_position;
		previous_position = knot.position();
	}
	if (!valid) {
		report->addIssue({
			ParametricVehicleValidationCode::InvalidStationFunction,
			"Station functions require a purpose identifier and strictly ordered finite knots in [0, 1].",
			{variant_identifier, function.identifier()}});
	}
}

bool finite_point(const glm::dvec3 &point)
{
	return std::isfinite(point.x) && std::isfinite(point.y) &&
	       std::isfinite(point.z);
}

bool finite_point(const glm::dvec2 &point)
{
	return std::isfinite(point.x) && std::isfinite(point.y);
}

} // namespace

ParametricVehicleValidationReport
ModernCarParametricValidationService::validateObjectModel(
	const ModernCarParametricObjectModel &object_model) const
{
	ParametricVehicleValidationReport report;
	if (object_model.coordinateFrame().identifier() !=
	    "MCP_OMv1.VehicleCoordinateFrame") {
		report.addIssue({
			ParametricVehicleValidationCode::InvalidCoordinateFrame,
			"MCP_OMv1 requires the declared right/up/forward vehicle coordinate frame.",
			{object_model.coordinateFrame().identifier()}});
	}
	if (object_model.sourceManifest().deterministicHash().empty() ||
	    object_model.sourceManifest().files().empty()) {
		report.addIssue({
			ParametricVehicleValidationCode::MissingProvenance,
			"The parametric source manifest must identify source files and a deterministic hash.",
			{object_model.sourceManifest().generatorIdentifier()}});
	}
	std::set<std::string> identifiers;
	for (const ModernCarVariantDefinition &variant : object_model.family().variants()) {
		if (!identifiers.insert(variant.identifier()).second) {
			report.addIssue({
				ParametricVehicleValidationCode::InvalidPackage,
				"Modern-car variant identifiers must be unique.",
				{variant.identifier()}});
		}
		report.append(validateVariant(variant));
	}
	if (object_model.family().variants().size() != 4u) {
		report.addIssue({
			ParametricVehicleValidationCode::InvalidPackage,
			"MCP_OMv1 requires exactly four generated source variants.",
			{object_model.family().identifier()}});
	}
	report.append(validateGenerationPolicy(object_model.generationPolicy()));
	for (const GeneratedVehicleRealization &realization : object_model.realizations()) {
		const ModernCarVariantDefinition *variant =
			object_model.family().findVariant(realization.variantIdentifier());
		if (variant == nullptr) {
			report.addIssue({
				ParametricVehicleValidationCode::MissingProvenance,
				"A generated realization refers to an unknown source variant.",
				{realization.identifier(), realization.variantIdentifier()}});
			continue;
		}
		report.append(validateRealization(*variant, realization));
	}
	return report;
}

ParametricVehicleValidationReport
ModernCarParametricValidationService::validateVariant(
	const ModernCarVariantDefinition &variant) const
{
	ParametricVehicleValidationReport report;
	const VehiclePackageParameters &package = variant.package();
	const bool package_values_valid =
		!variant.identifier().empty() && !package.identifier().empty() &&
		finite_positive(package.length()) && finite_positive(package.width()) &&
		finite_positive(package.height()) && finite_positive(package.wheelbase()) &&
		finite_nonnegative(package.groundClearance()) &&
		finite_nonnegative(package.frontOverhang()) &&
		finite_nonnegative(package.rearOverhang());
	const double package_equation_error = std::fabs(
		package.length() -
		(package.frontOverhang() + package.wheelbase() + package.rearOverhang()));
	if (!package_values_valid || package_equation_error > 1.0e-6) {
		report.addIssue({
			ParametricVehicleValidationCode::InvalidPackage,
			"Vehicle package dimensions must be finite, positive, and satisfy the overhang-wheelbase equation.",
			{variant.identifier(), package.identifier()}});
	}
	const VehicleWheelParameters &wheels = variant.wheels();
	if (!finite_positive(wheels.radius()) || !finite_positive(wheels.width()) ||
	    !finite_positive(wheels.frontTrack()) || !finite_positive(wheels.rearTrack()) ||
	    wheels.frontTrack() >= package.width() || wheels.rearTrack() >= package.width()) {
		report.addIssue({
			ParametricVehicleValidationCode::InvalidWheelParameters,
			"Wheel radius, width, and tracks must be positive; tracks must remain inside overall width.",
			{variant.identifier()}});
	}

	const VehicleLongitudinalDomain &domain = variant.body().longitudinalDomain();
	if (!(domain.rear() < domain.middle() && domain.middle() < domain.front() &&
	      domain.cabinRear() < domain.cabinMiddle() &&
	      domain.cabinMiddle() < domain.cabinFront() &&
	      domain.cabinRear() >= domain.rear() && domain.cabinFront() <= domain.front())) {
		report.addIssue({
			ParametricVehicleValidationCode::InvalidFieldDefinition,
			"Body and greenhouse longitudinal domains must be strictly ordered and nested.",
			{variant.identifier()}});
	}
	const LowerBodyFieldDefinition &lower = variant.body().lowerBody();
	const GreenhouseFieldDefinition &greenhouse = variant.body().greenhouse();
	const FenderFieldDefinition &fenders = variant.body().fenders();
	if (!finite_positive(lower.longitudinalExponent()) ||
	    !finite_positive(lower.lateralExponent()) ||
	    !finite_positive(lower.verticalExponent()) ||
	    !finite_positive(lower.widthScale()) ||
	    !finite_positive(greenhouse.longitudinalExponent()) ||
	    !finite_positive(greenhouse.lateralExponent()) ||
	    !finite_positive(greenhouse.verticalExponent()) ||
	    !finite_positive(greenhouse.widthScale()) ||
	    !finite_positive(greenhouse.roofCrownLongitudinalExponent()) ||
	    !finite_positive(greenhouse.roofCrownVerticalExponent()) ||
	    !finite_positive(fenders.frontLongitudinalRadius()) ||
	    !finite_positive(fenders.rearLongitudinalRadius()) ||
	    !finite_positive(fenders.frontVerticalRadius()) ||
	    !finite_positive(fenders.rearVerticalRadius()) ||
	    !finite_positive(fenders.halfWidth()) ||
	    !finite_positive(variant.body().blend().lowerGreenhouseSmoothness()) ||
	    !finite_positive(variant.body().blend().fenderSmoothness()) ||
	    !finite_nonnegative(variant.body().wheelhouseDifference().clearance())) {
		report.addIssue({
			ParametricVehicleValidationCode::InvalidFieldDefinition,
			"Implicit-field dimensions, exponents, scales, and blend smoothness must be finite and valid.",
			{variant.identifier()}});
	}
	for (const StationFunction *function : {
		&lower.widthRatioByStation(), &lower.centreHeightByStation(),
		&lower.halfHeightByStation(), &greenhouse.widthRatioByStation(),
		&greenhouse.centreHeightByStation(), &greenhouse.halfHeightByStation()}) {
		validate_station_function(*function, &report, variant.identifier());
	}
	if (variant.body().characterCurveNetwork().curves().size() != 8u ||
	    variant.body().characterCurveNetwork().samplesPerCurve() == 0u ||
	    variant.body().bodySectionNetwork().sectionCount() == 0u) {
		report.addIssue({
			ParametricVehicleValidationCode::InvalidFieldDefinition,
			"Each source variant must define eight character curves and a non-empty section network.",
			{variant.identifier()}});
	}

	const VehiclePowertrainIntent &powertrain = variant.powertrain();
	bool powertrain_valid =
		powertrain.driveIntent() == ModernCarDriveIntent::AllWheelDrive &&
		powertrain.drivesFrontAxle() && powertrain.drivesRearAxle();
	if (powertrain.powerSource() == ModernCarPowerSource::BatteryElectric) {
		powertrain_valid = powertrain_valid &&
			powertrain.energyStorageEnvelope().isFiniteAndOrdered() &&
			!powertrain.energyStorageEnvelope().identifier().empty() &&
			powertrain.exhaustOutletCount() == 0 &&
			variant.exteriorFeatures().exhaustOutletCount() == 0;
	}
	else {
		powertrain_valid = powertrain_valid &&
			powertrain.energyStorageEnvelope().identifier().empty();
	}
	if (!powertrain_valid) {
		report.addIssue({
			ParametricVehicleValidationCode::InvalidPowertrainIntent,
			"AWD and energy-source intent must agree with axle, storage, and exhaust evidence.",
			{variant.identifier()}});
	}
	return report;
}

ParametricVehicleValidationReport
ModernCarParametricValidationService::validateGenerationPolicy(
	const ParametricModelGenerationPolicy &policy) const
{
	ParametricVehicleValidationReport report;
	if (policy.identifier().empty() || policy.longitudinalSamples() < 3u ||
	    policy.lateralSamples() < 3u || policy.verticalSamples() < 3u ||
	    !std::isfinite(policy.isoValue()) || policy.allowsStationExtrapolation()) {
		report.addIssue({
			ParametricVehicleValidationCode::InvalidGenerationPolicy,
			"Generation policy must identify a bounded grid and disallow implicit station extrapolation.",
			{policy.identifier()}});
	}
	return report;
}

ParametricVehicleValidationReport
ModernCarParametricValidationService::validateRealization(
	const ModernCarVariantDefinition &variant,
	const GeneratedVehicleRealization &realization) const
{
	ParametricVehicleValidationReport report;
	if (realization.variantIdentifier() != variant.identifier() ||
	    realization.sourceManifestHash() == 0u ||
	    realization.generationPolicyHash() == 0u ||
	    realization.deterministicGeometryHash() == 0u) {
		report.addIssue({
			ParametricVehicleValidationCode::MissingProvenance,
			"Generated realization must retain source variant, source hash, policy hash, and geometry hash.",
			{realization.identifier(), realization.variantIdentifier()}});
	}
	const GeneratedBodyMesh &body_mesh = realization.bodyMesh();
	bool mesh_is_finite = body_mesh.mesh() != nullptr &&
		!body_mesh.mesh()->vertices.empty() && !body_mesh.mesh()->faces.empty();
	if (body_mesh.mesh()) {
		for (const glm::vec3 &vertex : body_mesh.mesh()->vertices) {
			mesh_is_finite = mesh_is_finite && finite_point(glm::dvec3(vertex));
		}
	}
	if (!mesh_is_finite) {
		report.addIssue({
			ParametricVehicleValidationCode::NonfiniteGeneratedMesh,
			"Generated body mesh must contain finite vertices and indexed faces.",
			{realization.identifier()}});
	}
	const glm::dvec3 expected_minimum(
		-variant.package().width() * 0.5,
		0.0,
		-variant.package().length() * 0.5);
	const glm::dvec3 expected_maximum(
		variant.package().width() * 0.5,
		variant.package().height(),
		variant.package().length() * 0.5);
	if (!finite_point(body_mesh.boundsMinimum()) ||
	    !finite_point(body_mesh.boundsMaximum()) ||
	    glm::length(body_mesh.boundsMinimum() - expected_minimum) > 1.0e-4 ||
	    glm::length(body_mesh.boundsMaximum() - expected_maximum) > 1.0e-4) {
		report.addIssue({
			ParametricVehicleValidationCode::PackageBoundsMismatch,
			"Generated MCP body bounds must match package width, height, and length.",
			{realization.identifier()}});
	}
	if (!body_mesh.isWatertight()) {
		report.addIssue({
			ParametricVehicleValidationCode::NonWatertightGeneratedMesh,
			"Generated body topology is not watertight.",
			{realization.identifier()}});
	}
	if (realization.characterCurves().curves().size() != 8u) {
		report.addIssue({
			ParametricVehicleValidationCode::MissingCharacterCurves,
			"Generated realization must contain eight semantic character curves.",
			{realization.identifier()}});
	}
	if (realization.bodySections().sections().size() !=
	    variant.body().bodySectionNetwork().sectionCount()) {
		report.addIssue({
			ParametricVehicleValidationCode::MissingBodySections,
			"Generated realization does not contain the declared body section count.",
			{realization.identifier()}});
	}
	if (realization.observations().observations().size() != 5u) {
		report.addIssue({
			ParametricVehicleValidationCode::MissingObservations,
			"Generated realization must contain front, rear, left, right, and top observations.",
			{realization.identifier()}});
	}
	for (const GeneratedVehicleObservation &observation :
	     realization.observations().observations()) {
		for (const glm::dvec2 &point : observation.normalizedSilhouette()) {
			if (!finite_point(point) || point.x < 0.0 || point.x > 1.0 ||
			    point.y < 0.0 || point.y > 1.0) {
				report.addIssue({
					ParametricVehicleValidationCode::MissingObservations,
					"Observation silhouettes must be finite and normalized to [0, 1].",
					{realization.identifier(), observation.identifier()}});
				break;
			}
		}
	}
	return report;
}
