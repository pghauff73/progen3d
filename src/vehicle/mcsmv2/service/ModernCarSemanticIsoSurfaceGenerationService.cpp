#include "vehicle/mcsmv2/service/ModernCarSemanticIsoSurfaceGenerationService.h"

#include "geometry/model/ImplicitSurfaceGenerationRequest.h"
#include "geometry/service/ClosedSurfaceFaceOrientationService.h"
#include "geometry/service/ImplicitSurfaceMeshingService.h"
#include "geometry/service/MeshTopologyAnalyzer.h"
#include "vehicle/mcsmv2/model/InverseAffinePrewarpedField.h"
#include "vehicle/mcsmv2/service/ImplicitFieldCalibrationResolutionService.h"
#include "vehicle/mcsmv2/service/ModernCarSemanticBodyFieldFactory.h"

#include <glm/common.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace {

ImplicitFieldBounds calculateMeshBounds(const Mesh &mesh)
{
	glm::dvec3 minimum(std::numeric_limits<double>::infinity());
	glm::dvec3 maximum(-std::numeric_limits<double>::infinity());
	for (const glm::vec3 &vertex : mesh.vertices) {
		minimum = glm::min(minimum, glm::dvec3(vertex));
		maximum = glm::max(maximum, glm::dvec3(vertex));
	}
	return ImplicitFieldBounds(minimum, maximum);
}

} // namespace

GeneratedModernCarSemanticBodyResult
ModernCarSemanticIsoSurfaceGenerationService::generate(
	const ModernCarSemanticVariant &variant,
	const ParametricModelGenerationPolicy &policy) const
{
	ModernCarSemanticValidationReport validation;
	if (variant.fieldCalibration().postMeshAffineCorrectionApplied() ||
	    variant.fieldCalibration().maximumPostMeshCorrectionFraction() != 0.0) {
		validation.addIssue({
			ModernCarSemanticValidationCode::InvalidSourceCatalog,
			"MCSMv2 source calibration must prohibit post-mesh affine correction.",
			{variant.identifier()}});
		return GeneratedModernCarSemanticBodyResult(
			std::nullopt, std::move(validation));
	}
	const std::shared_ptr<const ImplicitScalarField> body_field =
		ModernCarSemanticBodyFieldFactory().createBodyField(variant);
	const ImplicitSurfaceGenerationRequest request(
		policy.identifier(),
		policy.longitudinalSamples(),
		policy.lateralSamples(),
		policy.verticalSamples(),
		policy.isoValue(),
		1,
		0.38,
		-0.41,
		GeometryDetailLevel::CoarseShape);
	const ImplicitFieldCalibrationResolutionResult calibration_result =
		ImplicitFieldCalibrationResolutionService().resolve(
			body_field, variant.fieldCalibration(), request);
	if (!calibration_result.succeeded() || !calibration_result.calibration()) {
		validation.addIssue({
			ModernCarSemanticValidationCode::NonfiniteMesh,
			calibration_result.diagnostic(),
			{variant.identifier()}});
		return GeneratedModernCarSemanticBodyResult(
			std::nullopt, std::move(validation));
	}
	const ImplicitFieldCalibration resolved_calibration =
		*calibration_result.calibration();
	if (!(resolved_calibration.maximumPrewarpFraction() < 0.02)) {
		validation.addIssue({
			ModernCarSemanticValidationCode::PackageBoundsMismatch,
			"MCSMv2 inverse field prewarp exceeds the accepted two-percent limit.",
			{variant.identifier()}});
		return GeneratedModernCarSemanticBodyResult(
			std::nullopt, std::move(validation));
	}
	const std::shared_ptr<const ImplicitScalarField> calibrated_body_field =
		std::make_shared<InverseAffinePrewarpedField>(
			body_field, resolved_calibration);
	const ImplicitSurfaceGenerationResult surface_result =
		ImplicitSurfaceMeshingService().generate(*calibrated_body_field, request);
	if (!surface_result.succeeded() || !surface_result.generatedMesh() ||
	    !surface_result.generatedMesh()->mesh()) {
		validation.addIssue({
			ModernCarSemanticValidationCode::NonfiniteMesh,
			surface_result.firstDiagnostic().empty()
				? "MCSMv2 implicit field did not produce a mesh."
				: surface_result.firstDiagnostic(),
			{variant.identifier()}});
		return GeneratedModernCarSemanticBodyResult(
			std::nullopt, std::move(validation));
	}

	Mesh source_mesh = *surface_result.generatedMesh()->mesh();
	const ImplicitFieldBounds source_bounds = surface_result.generatedBounds();
	const glm::dvec3 minimum_error = glm::abs(
		source_bounds.minimum() - resolved_calibration.targetBoundsMinimum());
	const glm::dvec3 maximum_error = glm::abs(
		source_bounds.maximum() - resolved_calibration.targetBoundsMaximum());
	const glm::dvec3 tolerance(resolved_calibration.packageTolerance());
	if (!glm::all(glm::lessThanEqual(minimum_error, tolerance)) ||
	    !glm::all(glm::lessThanEqual(maximum_error, tolerance))) {
		validation.addIssue({
			ModernCarSemanticValidationCode::PackageBoundsMismatch,
			"MCSMv2 calibrated body bounds exceed the accepted package tolerance.",
			{variant.identifier()}});
		return GeneratedModernCarSemanticBodyResult(
			std::nullopt, std::move(validation));
	}
	for (glm::vec3 &vertex : source_mesh.vertices) {
		vertex = glm::vec3(
			variant.referenceFrame().convertSourcePointToProgen3d(
				glm::dvec3(vertex)));
	}
	const ClosedSurfaceFaceOrientationReport orientation_report =
		ClosedSurfaceFaceOrientationService().orientOutward(source_mesh);
	if (!orientation_report.succeeded()) {
		validation.addIssue({
			ModernCarSemanticValidationCode::InconsistentMeshOrientation,
			orientation_report.diagnostic(),
			{variant.identifier()}});
		return GeneratedModernCarSemanticBodyResult(
			std::nullopt, std::move(validation));
	}
	const ImplicitFieldBounds progen_bounds = calculateMeshBounds(source_mesh);
	const std::vector<MeshSurfaceTag> canonical_surface_tags(
		source_mesh.faces.size(), MeshSurfaceTag(MeshSurfaceRole::Outer));
	const MeshTopologyReport topology =
		MeshTopologyAnalyzer().analyze(source_mesh, canonical_surface_tags);
	if (!topology.isWatertight()) {
		validation.addIssue({
			ModernCarSemanticValidationCode::NonWatertightMesh,
			"MCSMv2 body mesh is not watertight.",
			{variant.identifier()}});
	}
	std::shared_ptr<const Mesh> immutable_mesh =
		std::make_shared<Mesh>(std::move(source_mesh));
	return GeneratedModernCarSemanticBodyResult(
		GeneratedModernCarSemanticBody(
			GeneratedBodyMesh(
				std::move(immutable_mesh),
				progen_bounds.minimum(),
				progen_bounds.maximum(),
				topology.isWatertight(),
				topology.topology_hash),
			resolved_calibration),
		std::move(validation));
}
