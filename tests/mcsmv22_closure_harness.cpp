#include "vehicle/mcsmv2/service/McsMv22ClosureKinematicEvaluationService.h"
#include "vehicle/mcsmv2/service/McsMv21SemanticCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv21SemanticGeometryGenerationService.h"
#include "vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv22NativePartitionGeometryLoadingService.h"
#include "vehicle/mcsmv2/service/McsMv22SemanticKinematicBindingService.h"
#include "vehicle/mcsmv2/service/ModernCarSemanticCatalogFactory.h"
#include "vehicle/service/VehicleClosureMotionEvaluationService.h"
#include "vehicle/service/VehicleClosureSweepValidationService.h"

#include <glm/gtc/matrix_inverse.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <string>

namespace {

bool require(bool condition, const std::string &message)
{
	if (!condition) std::cerr << "FAIL: " << message << '\n';
	return condition;
}

bool isIdentity(const glm::dmat4 &matrix, double tolerance = 1.0e-12)
{
	for (std::size_t column = 0u; column < 4u; ++column) {
		for (std::size_t row = 0u; row < 4u; ++row) {
			const double expected = column == row ? 1.0 : 0.0;
			if (std::abs(matrix[column][row] - expected) > tolerance) return false;
		}
	}
	return true;
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
		"MCSMv2.2.ClosureValidation.PreviewBody",
		ParametricGenerationResolution::Low, 52u, 32u, 32u, 0.0, true);
	const ParametricModelGenerationPolicy registration_policy(
		"MCSMv2.2.ClosureValidation.Registration",
		ParametricGenerationResolution::Low, 113u, 65u, 81u, 0.0, true);
	const GeneratedMcsMv21SemanticGeometryResult result =
		McsMv21SemanticGeometryGenerationService().generate(
			findBaseVariant(base_family, identifier),
			findSemanticVariant(semantic_family, identifier),
			body_policy, registration_policy, 201u);
	if (!result.succeeded() || !result.generatedGeometry()) {
		throw std::runtime_error("MCSMv2.1 semantic generation failed.");
	}
	return std::make_shared<McsMv21SemanticGeometry>(*result.generatedGeometry());
}

} // namespace

int main()
{
	const McsMv22KinematicFamilyDefinition family =
		McsMv22KinematicCatalogFactory().createFamilyDefinition();
	const McsMv22ClosureKinematicEvaluationService mcsm_service;
	const VehicleClosureMotionEvaluationService motion_service;
	const VehicleClosureSweepValidationService validation_service;
	bool passed = true;

	for (const McsMv22KinematicVariantDefinition &variant : family.variants()) {
		const std::shared_ptr<McsMv21SemanticGeometry> semantic_geometry =
			generateSemanticGeometry(variant.identifier());
		const McsMv22SemanticKinematicBindings semantic_bindings =
			McsMv22SemanticKinematicBindingService().bind(
				*semantic_geometry, variant);
		const McsMv22NativePartitionGeometry evidence_geometry =
			McsMv22NativePartitionGeometryLoadingService().loadProgen3dGeometry(
				"examples/MVPv2_5_Red_AWD_Hatchback/"
				"MCSMv2_2_0_NATIVE_EVIDENCE/partition_meshes",
				variant.identifier(), family.sourceRelease().toProgen3dMatrix());
		const VehicleClosureSweepSet sweeps =
			mcsm_service.createClosureSweepSet(variant);
		passed &= require(
			sweeps.sweeps().size() == 6u,
			variant.identifier() + " must bind six closure sweeps");
		std::set<std::string> closure_identifiers;
		std::size_t sampled_state_count = 0u;
		for (const VehicleClosureSweep &sweep : sweeps.sweeps()) {
			closure_identifiers.insert(sweep.surfaceBinding().closureIdentifier());
			passed &= require(
				sweep.surfaceBinding().surfaceDomainIdentifier() ==
					sweep.surfaceBinding().closureIdentifier(),
				variant.identifier() + " closure must bind its exact semantic domain");
			passed &= require(
				sweep.surfaceBinding().sourceFaceCount() > 0u,
				variant.identifier() + " closure binding must own source faces");
			passed &= require(
				sweep.poseSamples().size() == 7u,
				variant.identifier() + " closure must contain seven accepted states");
			sampled_state_count += sweep.poseSamples().size();

			const VehicleClosurePoseSample closed =
				motion_service.evaluateClosurePose(
					sweep.surfaceBinding().joint(), 0.0);
			passed &= require(
				isIdentity(closed.sourceTransform()),
				variant.identifier() + " closed closure transform must be identity");
			const VehicleClosurePoseSample &fully_open = sweep.poseSamples().back();
			passed &= require(
				!isIdentity(fully_open.sourceTransform(), 1.0e-9),
				variant.identifier() + " fully open closure must move");
			passed &= require(
				std::abs(
					glm::determinant(glm::dmat3(fully_open.sourceTransform())) - 1.0) <
					1.0e-10,
				variant.identifier() + " closure transform must remain rigid");
			passed &= require(
				std::abs(
					fully_open.angleDegrees() -
					sweep.surfaceBinding().joint().direction() *
						sweep.surfaceBinding().joint().maximumAngleDegrees()) <
					1.0e-12,
					variant.identifier() + " full closure angle must match source");

			const auto closure_mesh = evidence_geometry.closureMeshes().find(
				sweep.surfaceBinding().closureIdentifier());
			passed &= require(
				closure_mesh != evidence_geometry.closureMeshes().end(),
				variant.identifier() + " closure mesh must be bound");
			if (closure_mesh == evidence_geometry.closureMeshes().end()) continue;
			const VehicleClosureSweep validated_sweep =
				validation_service.validateAndBuildSweep(
					sweep, closure_mesh->second, evidence_geometry.fixedBodyMesh(),
					family.sourceRelease().toProgen3dMatrix());
			double minimum_non_hinge_distance =
				std::numeric_limits<double>::infinity();
			std::size_t intersection_count = 0u;
			for (const VehicleClosurePoseSample &sample :
			     validated_sweep.poseSamples()) {
				minimum_non_hinge_distance = std::min(
					minimum_non_hinge_distance,
					sample.validation().nonHingeDistance().minimumDistance());
				intersection_count +=
					sample.validation().intersectionScreening().intersectionCount();
				passed &= require(
					sample.validation().intersectionScreening().complete(),
					variant.identifier() + " closure screening must be complete");
			}
			std::cout << "variant=" << variant.identifier()
			          << " closure=" << sweep.surfaceBinding().closureIdentifier()
			          << " minimum_non_hinge_distance="
			          << minimum_non_hinge_distance
			          << " intersections=" << intersection_count
			          << " passed=" << (validated_sweep.passed() ? "true" : "false")
			          << '\n';
			passed &= require(
				validated_sweep.passed(),
				variant.identifier() + " closure sweep must pass independent validation");
		}
		passed &= require(
			closure_identifiers.size() == 6u,
			variant.identifier() + " closure identifiers must be unique");
		passed &= require(
			sampled_state_count == 42u,
			variant.identifier() + " must evaluate 42 closure states");

		const VehicleJointGraph graph = mcsm_service.createJointGraph(variant);
		passed &= require(
			graph.joints().size() == 10u,
			variant.identifier() + " joint graph must contain six closures and four glass joints");
		std::size_t revolute_count = 0u;
		std::size_t prismatic_count = 0u;
		for (const VehicleJoint &joint : graph.joints()) {
			if (joint.type() == VehicleJointType::Revolute) ++revolute_count;
			if (joint.type() == VehicleJointType::Prismatic) ++prismatic_count;
		}
		passed &= require(revolute_count == 6u, variant.identifier() + " must register six revolute joints");
		passed &= require(prismatic_count == 4u, variant.identifier() + " must register four glass carrier joints");
	}

	if (!passed) return 1;
	std::cout << "MCSMv2.2 closure motion checks passed.\n";
	return 0;
}
