#include "vehicle/parametric/service/ModernCarIsoSurfaceGenerationService.h"

#include "geometry/model/ImplicitScalarField.h"
#include "geometry/model/ImplicitSurfaceGenerationRequest.h"
#include "geometry/service/ImplicitSurfaceMeshingService.h"
#include "geometry/service/MeshTopologyAnalyzer.h"
#include "vehicle/parametric/service/ModernCarFieldEvaluationService.h"

#include <glm/common.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

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

class ModernCarBodyImplicitField final : public ImplicitScalarField
{
public:
	explicit ModernCarBodyImplicitField(const ModernCarVariantDefinition &variant)
		: variant_(variant)
	{
	}

	double evaluateAt(const glm::dvec3 &position) const override
	{
		return field_evaluation_service_.evaluateBodyField(
			variant_, McsM1Coordinate(position));
	}

	ImplicitFieldBounds evaluationBounds() const override
	{
		const VehicleLongitudinalDomain &domain =
			variant_.body().longitudinalDomain();
		const double longitudinal_margin =
			std::max(0.10, variant_.package().length() * 0.05);
		return ImplicitFieldBounds(
			glm::dvec3(
				domain.rear() - longitudinal_margin,
				-variant_.package().width() * 0.60,
				-0.12),
			glm::dvec3(
				domain.front() + longitudinal_margin,
				variant_.package().width() * 0.60,
				variant_.package().height() + 0.08));
	}

	std::uint64_t deterministicHash() const override
	{
		std::uint64_t hash = 1469598103934665603ull;
		for (unsigned char character : variant_.identifier()) {
			hash ^= character;
			hash *= 1099511628211ull;
		}
		return hash;
	}

private:
	const ModernCarVariantDefinition &variant_;
	ModernCarFieldEvaluationService field_evaluation_service_;
};

} // namespace

GeneratedBodyMeshResult ModernCarIsoSurfaceGenerationService::generate(
	const ModernCarVariantDefinition &variant,
	const ParametricModelGenerationPolicy &policy) const
{
	ParametricVehicleValidationReport validation;
	const ModernCarBodyImplicitField field(variant);
	const ImplicitSurfaceGenerationRequest request(
		policy.identifier(),
		policy.longitudinalSamples(),
		policy.lateralSamples(),
		policy.verticalSamples(),
		policy.isoValue(),
		3,
		0.45,
		-0.48,
		GeometryDetailLevel::CoarseShape);
	const ImplicitSurfaceGenerationResult surface_result =
		ImplicitSurfaceMeshingService().generate(field, request);
	if (!surface_result.succeeded() || !surface_result.generatedMesh() ||
	    !surface_result.generatedMesh()->mesh()) {
		validation.addIssue({
			ParametricVehicleValidationCode::NonfiniteGeneratedMesh,
			surface_result.firstDiagnostic().empty()
				? "Field evaluation did not produce a finite body mesh."
				: surface_result.firstDiagnostic(),
			{variant.identifier()}});
		return GeneratedBodyMeshResult(std::nullopt, std::move(validation));
	}

	Mesh source_mesh = *surface_result.generatedMesh()->mesh();
	const ImplicitFieldBounds source_bounds =
		surface_result.generatedBounds();
	const glm::dvec3 source_dimensions = source_bounds.dimensions();
	if (!source_bounds.isValid()) {
		validation.addIssue({
			ParametricVehicleValidationCode::PackageBoundsMismatch,
			"Generated source mesh has a collapsed axis.",
			{variant.identifier()}});
		return GeneratedBodyMeshResult(std::nullopt, std::move(validation));
	}

	const glm::dvec3 source_target_center(
		variant.package().sourcePackageCenterStation(),
		0.0,
		variant.package().height() * 0.5);
	const glm::dvec3 source_scale(
		variant.package().length() / source_dimensions.x,
		variant.package().width() / source_dimensions.y,
		variant.package().height() / source_dimensions.z);
	const glm::dvec3 source_center =
		(source_bounds.minimum() + source_bounds.maximum()) * 0.5;
	ModernCarCoordinateFrame coordinate_frame;
	for (glm::vec3 &vertex : source_mesh.vertices) {
		const glm::dvec3 normalized_source =
			(glm::dvec3(vertex) - source_center) * source_scale +
			source_target_center;
		vertex = glm::vec3(coordinate_frame.convertSourceToMcp(
			McsM1Coordinate(normalized_source),
			variant.package().sourcePackageCenterStation()).value());
	}
	source_mesh.calc_normals();

	const ImplicitFieldBounds mcp_bounds = calculateMeshBounds(source_mesh);
	const MeshTopologyReport topology =
		MeshTopologyAnalyzer().analyze(source_mesh, {});
	std::shared_ptr<const Mesh> immutable_mesh =
		std::make_shared<Mesh>(std::move(source_mesh));
	return GeneratedBodyMeshResult(
		GeneratedBodyMesh(
			std::move(immutable_mesh),
			mcp_bounds.minimum(),
			mcp_bounds.maximum(),
			topology.isWatertight(),
			topology.topology_hash),
		std::move(validation));
}
