#include "vehicle/mcsmv2/service/McsMv22ClosureKinematicEvaluationService.h"
#include "vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv22NativePartitionGeometryLoadingService.h"
#include "vehicle/service/VehicleClosureMotionEvaluationService.h"
#include "vehicle/service/VehicleGlassMotionValidationService.h"
#include "vehicle/service/VehicleReferenceFrameTransformationService.h"

#include <glm/gtc/matrix_inverse.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace {

bool require(bool condition, const std::string &message)
{
	if (!condition) std::cerr << "FAIL: " << message << '\n';
	return condition;
}

bool closeMatrix(
	const glm::dmat4 &actual,
	const glm::dmat4 &expected,
	double tolerance = 1.0e-12)
{
	for (std::size_t column = 0u; column < 4u; ++column) {
		for (std::size_t row = 0u; row < 4u; ++row) {
			if (std::abs(actual[column][row] - expected[column][row]) > tolerance) {
				return false;
			}
		}
	}
	return true;
}

bool isIdentity(const glm::dmat4 &matrix, double tolerance = 1.0e-12)
{
	return closeMatrix(matrix, glm::dmat4(1.0), tolerance);
}

} // namespace

int main()
{
	const McsMv22KinematicFamilyDefinition family =
		McsMv22KinematicCatalogFactory().createFamilyDefinition();
	const McsMv22ClosureKinematicEvaluationService mcsm_service;
	const VehicleClosureMotionEvaluationService motion_service;
	const VehicleReferenceFrameTransformationService frame_service;
	const VehicleGlassMotionValidationService validation_service;
	bool passed = true;

	for (const McsMv22KinematicVariantDefinition &variant : family.variants()) {
		const double parent_closure_state = 0.55;
		const VehicleClosureSweepSet closure_sweeps =
			mcsm_service.createClosureSweepSet(variant);
		const VehicleGlassMotionSet glass_motions =
			mcsm_service.createGlassMotionSet(variant, parent_closure_state);
		const McsMv22NativePartitionGeometry evidence_geometry =
			McsMv22NativePartitionGeometryLoadingService().loadProgen3dGeometry(
				"examples/MVPv2_5_Red_AWD_Hatchback/"
				"MCSMv2_2_0_NATIVE_EVIDENCE/partition_meshes",
				variant.identifier(), family.sourceRelease().toProgen3dMatrix());
		passed &= require(
			glass_motions.motions().size() == 4u,
			variant.identifier() + " must bind four moving side-glass systems");
		std::size_t sampled_state_count = 0u;

		for (const VehicleGlassMotion &glass_motion : glass_motions.motions()) {
			const VehicleGlassSurfaceBinding &binding =
				glass_motion.surfaceBinding();
			passed &= require(
				binding.apertureDomainIdentifier() == binding.motion().identifier(),
				variant.identifier() + " glass must bind its exact aperture domain");
			passed &= require(
				binding.sourceFaceCount() > 0u,
				variant.identifier() + " glass binding must own source faces");
			passed &= require(
				glass_motion.poseSamples().size() == 6u,
				variant.identifier() + " glass must contain six accepted states");
			sampled_state_count += glass_motion.poseSamples().size();
			const VehicleGlassPoseSample &closed = glass_motion.poseSamples().front();
			const VehicleGlassPoseSample &fully_lowered = glass_motion.poseSamples().back();
			passed &= require(
				isIdentity(closed.localSourceTransform()),
				variant.identifier() + " glass state zero must be parent-relative identity");
			passed &= require(
				fully_lowered.localSourceTransform()[3].z < -0.4,
				variant.identifier() + " full glass state must lower in source Z");
			passed &= require(
				std::abs(
					fully_lowered.verticalDropMetres() -
					binding.motion().travelMetres()) < 1.0e-12,
				variant.identifier() + " full glass drop must match source travel");

			const VehicleClosureSweep *parent_sweep = nullptr;
			for (const VehicleClosureSweep &candidate : closure_sweeps.sweeps()) {
				if (candidate.surfaceBinding().closureIdentifier() ==
				    binding.parentClosureIdentifier()) {
					parent_sweep = &candidate;
					break;
				}
			}
			passed &= require(
				parent_sweep != nullptr,
				variant.identifier() + " glass parent closure must exist");
			if (parent_sweep == nullptr) continue;
			const glm::dmat4 parent_transform = motion_service
				.evaluateClosurePose(
					parent_sweep->surfaceBinding().joint(), parent_closure_state)
					.sourceTransform();
			passed &= require(
				closeMatrix(
					fully_lowered.worldSourceTransform(),
					parent_transform * fully_lowered.localSourceTransform()),
				variant.identifier() + " glass world transform must be parent-relative");
			passed &= require(
				std::abs(
					glm::determinant(glm::dmat3(fully_lowered.worldSourceTransform())) -
					1.0) < 1.0e-10,
				variant.identifier() + " composed glass transform must remain rigid");

			const glm::dmat4 target_local = frame_service.transformRigidBodyPose(
				fully_lowered.localSourceTransform(),
				family.sourceRelease().toProgen3dMatrix());
			passed &= require(
				target_local[3].y < -0.4,
					variant.identifier() + " full glass state must lower in ProGen Y");

			const auto glass_mesh = evidence_geometry.glassMeshes().find(
				binding.motion().identifier());
			const auto parent_closure_mesh = evidence_geometry.closureMeshes().find(
				binding.parentClosureIdentifier());
			passed &= require(
				glass_mesh != evidence_geometry.glassMeshes().end(),
				variant.identifier() + " glass evidence mesh must exist");
			passed &= require(
				parent_closure_mesh != evidence_geometry.closureMeshes().end(),
				variant.identifier() + " parent-door evidence mesh must exist");
			if (glass_mesh == evidence_geometry.glassMeshes().end() ||
			    parent_closure_mesh == evidence_geometry.closureMeshes().end()) {
				continue;
			}
			const VehicleGlassMotion validated_motion =
				validation_service.validateAndBuildMotion(
					glass_motion, glass_mesh->second, parent_closure_mesh->second,
					evidence_geometry.fixedBodyMesh(), parent_transform,
					family.sourceRelease().toProgen3dMatrix());
			double minimum_cavity_fraction = 1.0;
			double minimum_fixed_body_distance =
				std::numeric_limits<double>::infinity();
			double maximum_support_error = 0.0;
			std::size_t intersection_count = 0u;
			for (const VehicleGlassPoseSample &sample :
			     validated_motion.poseSamples()) {
				minimum_cavity_fraction = std::min(
					minimum_cavity_fraction,
					sample.validation().cavityContainmentFraction());
				minimum_fixed_body_distance = std::min(
					minimum_fixed_body_distance,
					sample.validation().fixedBodyDistance().minimumDistance());
				maximum_support_error = std::max(
					maximum_support_error,
					sample.validation().supportPoseCompositionError());
				intersection_count +=
					sample.validation().intersectionScreening().intersectionCount();
				passed &= require(
					sample.validation().intersectionScreening().complete(),
					variant.identifier() + " glass screening must be complete");
			}
			std::cout << "variant=" << variant.identifier()
			          << " glass=" << binding.motion().identifier()
			          << " minimum_cavity_fraction=" << minimum_cavity_fraction
			          << " minimum_fixed_body_distance="
			          << minimum_fixed_body_distance
			          << " maximum_support_error=" << maximum_support_error
			          << " intersections=" << intersection_count
			          << " passed=" << (validated_motion.passed() ? "true" : "false")
			          << '\n';
			passed &= require(
				validated_motion.passed(),
				variant.identifier() + " glass motion must pass independent validation");
		}
		passed &= require(
			sampled_state_count == 24u,
			variant.identifier() + " must evaluate 24 glass states");
	}

	if (!passed) return 1;
	std::cout << "MCSMv2.2 helical glass checks passed.\n";
	return 0;
}
