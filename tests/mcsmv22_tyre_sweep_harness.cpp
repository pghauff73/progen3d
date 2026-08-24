#include "vehicle/mcsmv2/service/McsMv21SemanticCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv21SemanticGeometryGenerationService.h"
#include "vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv22SemanticKinematicBindingService.h"
#include "vehicle/mcsmv2/service/McsMv22SuspensionKinematicEvaluationService.h"
#include "vehicle/mcsmv2/service/ModernCarSemanticBodyFieldFactory.h"
#include "vehicle/mcsmv2/service/ModernCarSemanticCatalogFactory.h"
#include "vehicle/service/VehicleReferenceFrameTransformationService.h"
#include "vehicle/service/VehicleTyreMeshGenerationService.h"
#include "vehicle/service/VehicleTyreSweepValidationService.h"
#include "geometry/service/MeshTopologyAnalyzer.h"
#include "geometry/service/StlMeshLoadingService.h"

#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

bool require(bool condition, const std::string &message)
{
	if (!condition) std::cerr << "FAIL: " << message << '\n';
	return condition;
}

const ModernCarSemanticVariant &findBaseVariant(
	const ModernCarSemanticFamily &family,
	const std::string &identifier)
{
	for (const ModernCarSemanticVariant &variant : family.variants()) {
		if (variant.identifier() == identifier) return variant;
	}
	throw std::runtime_error("Missing base semantic variant.");
}

const McsMv21SemanticVariantDefinition &findSemanticVariant(
	const McsMv21SemanticFamilyDefinition &family,
	const std::string &identifier)
{
	for (const McsMv21SemanticVariantDefinition &variant : family.variants()) {
		if (variant.identifier() == identifier) return variant;
	}
	throw std::runtime_error("Missing MCSMv2.1 semantic variant.");
}

std::shared_ptr<McsMv21SemanticGeometry> generateSemanticGeometry(
	const std::string &identifier)
{
	const ModernCarSemanticFamily base_family =
		ModernCarSemanticCatalogFactory().createFamily();
	const McsMv21SemanticFamilyDefinition semantic_family =
		McsMv21SemanticCatalogFactory().createFamilyDefinition();
	const ParametricModelGenerationPolicy body_policy(
		"MCSMv2.2.TyreValidation.PreviewBody", ParametricGenerationResolution::Low,
		52u, 32u, 32u, 0.0, true);
	const ParametricModelGenerationPolicy registration_policy(
		"MCSMv2.2.TyreValidation.Registration", ParametricGenerationResolution::Low,
		113u, 65u, 81u, 0.0, true);
	const GeneratedMcsMv21SemanticGeometryResult result =
		McsMv21SemanticGeometryGenerationService().generate(
			findBaseVariant(base_family, identifier),
			findSemanticVariant(semantic_family, identifier),
			body_policy, registration_policy, 201u);
	if (!result.succeeded() || !result.generatedGeometry()) {
		for (const ModernCarSemanticValidationIssue &issue :
		     result.validationReport().issues()) {
			std::cerr << "semantic_generation_issue=" << issue.message() << '\n';
		}
		throw std::runtime_error("MCSMv2.1 semantic generation failed.");
	}
	return std::make_shared<McsMv21SemanticGeometry>(*result.generatedGeometry());
}

std::size_t countImplicitBodyPenetrations(
	const GeneratedPrimitiveMesh &source_tyre,
	const std::vector<VehicleTyrePoseSample> &samples,
	const ModernCarSemanticVariant &base_variant)
{
	const std::shared_ptr<const ImplicitScalarField> body_field =
		ModernCarSemanticBodyFieldFactory().createBodyField(base_variant);
	std::size_t penetration_count = 0u;
	for (const VehicleTyrePoseSample &sample : samples) {
		for (const glm::vec3 &vertex : source_tyre.mesh()->vertices) {
			const glm::dvec3 source_position = glm::dvec3(
				sample.wheelPose().sourceTransform() * glm::dvec4(vertex, 1.0));
			if (body_field->evaluateAt(source_position) <= 0.0) ++penetration_count;
		}
	}
	return penetration_count;
}

std::pair<glm::dvec3, glm::dvec3> bounds(const Mesh &mesh)
{
	glm::dvec3 minimum(std::numeric_limits<double>::infinity());
	glm::dvec3 maximum(-std::numeric_limits<double>::infinity());
	for (const glm::vec3 &vertex : mesh.vertices) {
		minimum = glm::min(minimum, glm::dvec3(vertex));
		maximum = glm::max(maximum, glm::dvec3(vertex));
	}
	return {minimum, maximum};
}

} // namespace

int main()
{
	const McsMv22KinematicFamilyDefinition family =
		McsMv22KinematicCatalogFactory().createFamilyDefinition();
	const McsMv22KinematicVariantDefinition &variant = family.variants().front();
	const ModernCarSemanticFamily base_family =
		ModernCarSemanticCatalogFactory().createFamily();
	const ModernCarSemanticVariant &base_variant =
		findBaseVariant(base_family, variant.identifier());
	const std::shared_ptr<McsMv21SemanticGeometry> semantic =
		generateSemanticGeometry(variant.identifier());
	const McsMv22SemanticKinematicBindings bindings =
		McsMv22SemanticKinematicBindingService().bind(*semantic, variant);
	const std::vector<VehicleSuspensionCornerKinematicModel> corners =
		McsMv22SuspensionKinematicEvaluationService().createCornerModels(variant);
	const std::vector<VehicleWheelPoseEvaluation> evaluated_poses =
		McsMv22SuspensionKinematicEvaluationService().evaluateAcceptedPoseGrid(variant);
	const GeneratedPrimitiveMesh source_tyre =
		VehicleTyreMeshGenerationService().generateSourceFrameTyre(
			VehicleTyreMeshSpecification(
				variant.wheelRadiusMetres(), variant.wheelWidthMetres()));
	bool passed = true;
	std::map<VehicleCornerLocation, std::vector<VehicleTyrePoseSample>> samples;
	const VehicleReferenceFrameTransformationService frame_service;
	const GeneratedPrimitiveMesh source_body =
		StlMeshLoadingService().loadWeldedMesh(
			"examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_RELEASE/models/modern_car_v2_reference_body.stl");
	const MeshTopologyReport source_body_topology = MeshTopologyAnalyzer().analyze(
		*source_body.mesh(), source_body.faceSurfaceTags());
	passed &= require(
		source_body_topology.isWatertight(),
		"signed reference body STL must be watertight after deterministic welding");
	const auto body_bounds = bounds(*source_body.mesh());
	std::cout << "source_body_bounds="
	          << body_bounds.first.x << ',' << body_bounds.first.y << ',' << body_bounds.first.z
	          << " to "
	          << body_bounds.second.x << ',' << body_bounds.second.y << ',' << body_bounds.second.z
	          << '\n';
	for (const VehicleWheelPoseEvaluation &pose : evaluated_poses) {
		samples[pose.corner()].emplace_back(
			pose,
			frame_service.transformRigidBodyPose(
				pose.sourceTransform(), family.sourceRelease().toProgen3dMatrix()));
	}

	std::size_t total_pose_count = 0u;
	for (const VehicleSuspensionCornerKinematicModel &corner : corners) {
		const std::size_t implicit_penetration_count = countImplicitBodyPenetrations(
			source_tyre, samples[corner.corner()], base_variant);
		VehicleTyrePoseSweep sweep = VehicleTyreSweepValidationService().validateAndBuildSweep(
			corner.corner(), source_tyre, std::move(samples[corner.corner()]),
			*source_body.mesh(), glm::dmat4(1.0),
			variant.declaredTyreClearanceMetres(),
			variant.acceptedMinimumTyreClearanceMetres(),
			variant.tyreTessellationToleranceMetres());
		total_pose_count += sweep.poseSamples().size();
		passed &= require(
			sweep.validationReport().hullReport().passed(),
			"tyre sweep conservative hull must pass");
		passed &= require(
			sweep.validationReport().poseEvidence().size() == sweep.poseSamples().size(),
			"each tyre pose must own validation evidence");
		std::cout << "corner=" << static_cast<int>(corner.corner())
		          << " poses=" << sweep.poseSamples().size()
		          << " minimum_clearance=" << sweep.validationReport().minimumClearanceMetres()
		          << " implicit_penetrations=" << implicit_penetration_count
		          << " passed=" << (sweep.validationReport().passed() ? "true" : "false")
		          << '\n';
		if (!sweep.validationReport().passed()) {
			for (const VehicleTransformedTyrePoseEvidence &pose :
			     sweep.validationReport().poseEvidence()) {
				std::cout << "  steer=" << pose.wheelPose().steeringDegrees()
				          << " travel=" << pose.wheelPose().suspensionTravelMetres()
				          << " distance=" << pose.bodyDistance().minimumDistance()
				          << " candidates=" << pose.intersectionScreening().candidateTrianglePairCount()
				          << " intersections=" << pose.intersectionScreening().intersectionCount()
				          << " inside=" << pose.insideSampleCount()
				          << " passed=" << (pose.passed() ? "true" : "false")
				          << '\n';
			}
		}
		passed &= require(
			sweep.validationReport().passed(),
			"every reference tyre sweep must pass independent validation");
	}
	passed &= require(total_pose_count == 36u, "reference tyre sweep must contain 36 poses");
	if (!passed) return 1;
	std::cout << "MCSMv2.2 tyre sweep checks passed.\n";
	return 0;
}
