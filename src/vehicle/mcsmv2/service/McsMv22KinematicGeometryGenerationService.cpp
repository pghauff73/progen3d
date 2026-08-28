#include "vehicle/mcsmv2/service/McsMv22KinematicGeometryGenerationService.h"

#include "geometry/service/MeshTopologyAnalyzer.h"
#include "vehicle/service/VehicleClosureMotionEvaluationService.h"

#include <glm/gtc/matrix_inverse.hpp>

#include <cmath>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

bool close(double actual, double expected, double tolerance = 1.0e-12)
{
	return std::abs(actual - expected) <= tolerance;
}

bool closeVector(
	const glm::dvec3 &actual,
	const glm::dvec3 &expected,
	double tolerance = 1.0e-12)
{
	return close(actual.x, expected.x, tolerance) &&
	       close(actual.y, expected.y, tolerance) &&
	       close(actual.z, expected.z, tolerance);
}

bool closeMatrix(
	const glm::dmat4 &actual,
	const glm::dmat4 &expected,
	double tolerance = 1.0e-12)
{
	for (std::size_t column = 0u; column < 4u; ++column) {
		for (std::size_t row = 0u; row < 4u; ++row) {
			if (!close(actual[column][row], expected[column][row], tolerance)) {
				return false;
			}
		}
	}
	return true;
}

bool isRigid(const glm::dmat4 &transform)
{
	return std::abs(glm::determinant(glm::dmat3(transform)) - 1.0) <= 1.0e-10;
}

const VehicleClosureSweep *findClosureSweep(
	const VehicleClosureSweepSet &sweep_set,
	const std::string &closure_identifier)
{
	for (const VehicleClosureSweep &sweep : sweep_set.sweeps()) {
		if (sweep.surfaceBinding().closureIdentifier() == closure_identifier) {
			return &sweep;
		}
	}
	return nullptr;
}

std::string joinDiagnostics(const McsMv22KinematicAssuranceReport &report)
{
	std::ostringstream stream;
	for (std::size_t index = 0u; index < report.diagnostics().size(); ++index) {
		if (index != 0u) stream << ' ';
		stream << report.diagnostics()[index];
	}
	return stream.str();
}

} // namespace

GeneratedMcsMv22KinematicGeometryResult
McsMv22KinematicGeometryGenerationService::generate(
	std::shared_ptr<const McsMv21SemanticGeometry> semantic_geometry,
	const McsMv220SourceRelease &source_release,
	const McsMv22KinematicVariantDefinition &kinematic_definition,
	const McsMv22NativeKinematicEvidenceGeometry &evidence_geometry) const
{
	if (!semantic_geometry) {
		return GeneratedMcsMv22KinematicGeometryResult(
			std::nullopt, "MCSMv2.2 requires an inherited MCSMv2.1 semantic geometry.");
	}
	if (semantic_geometry->variantIdentifier() != kinematic_definition.identifier()) {
		return GeneratedMcsMv22KinematicGeometryResult(
			std::nullopt, "MCSMv2.1 and MCSMv2.2 variant identifiers do not match.");
	}
	if (evidence_geometry.variantIdentifier() != kinematic_definition.identifier()) {
		return GeneratedMcsMv22KinematicGeometryResult(
			std::nullopt, "MCSMv2.2 native evidence variant does not match the kinematic definition.");
	}

	try {
		const McsMv22SemanticKinematicBindings semantic_bindings =
			semantic_binding_service_.bind(*semantic_geometry, kinematic_definition);
		const std::vector<VehicleSuspensionCornerKinematicModel> corners =
			suspension_service_.createCornerModels(kinematic_definition);
		const std::vector<VehicleWheelPoseEvaluation> evaluated_poses =
			suspension_service_.evaluateAcceptedPoseGrid(kinematic_definition);

		bool wheel_pose_parity_passed =
			evaluated_poses.size() == kinematic_definition.acceptedWheelPoses().size();
		for (std::size_t pose_index = 0u;
		     pose_index < evaluated_poses.size() && wheel_pose_parity_passed;
		     ++pose_index) {
			const VehicleWheelPoseEvaluation &evaluated = evaluated_poses[pose_index];
			const McsMv22WheelPoseEvidence &accepted =
				kinematic_definition.acceptedWheelPoses()[pose_index];
			wheel_pose_parity_passed =
				close(evaluated.steeringDegrees(), accepted.steerDegrees()) &&
				close(evaluated.suspensionTravelMetres(), accepted.travelMetres()) &&
				close(evaluated.camberDegrees(), accepted.camberDegrees()) &&
				close(evaluated.toeDegrees(), accepted.toeDegrees()) &&
				closeVector(evaluated.sourceCentre(), accepted.sourceCentre()) &&
				closeMatrix(evaluated.sourceTransform(), accepted.sourceTransform()) &&
				isRigid(evaluated.sourceTransform());
		}

		const GeneratedPrimitiveMesh source_tyre =
			tyre_mesh_service_.generateSourceFrameTyre(
				VehicleTyreMeshSpecification(
					kinematic_definition.wheelRadiusMetres(),
					kinematic_definition.wheelWidthMetres()));
		const MeshTopologyReport tyre_topology = MeshTopologyAnalyzer().analyze(
			*source_tyre.mesh(), source_tyre.faceSurfaceTags());
		const bool tyre_topology_passed =
			source_tyre.mesh()->vertices.size() == 560u &&
			source_tyre.mesh()->faces.size() == 1120u && tyre_topology.isWatertight();

		std::map<VehicleCornerLocation, std::vector<VehicleTyrePoseSample>> tyre_samples;
		for (const VehicleWheelPoseEvaluation &pose : evaluated_poses) {
			tyre_samples[pose.corner()].emplace_back(
				pose,
				frame_service_.transformRigidBodyPose(
					pose.sourceTransform(), source_release.toProgen3dMatrix()));
		}
		std::vector<VehicleTyrePoseSweep> tyre_sweeps;
		tyre_sweeps.reserve(corners.size());
		bool native_tyre_validation_passed = corners.size() == 4u;
		std::size_t tyre_pose_count = 0u;
		for (const VehicleSuspensionCornerKinematicModel &corner : corners) {
			VehicleTyrePoseSweep sweep = tyre_validation_service_.validateAndBuildSweep(
				corner.corner(), source_tyre, std::move(tyre_samples[corner.corner()]),
				*evidence_geometry.sourceFinalBodyMesh().mesh(), glm::dmat4(1.0),
				kinematic_definition.declaredTyreClearanceMetres(),
				kinematic_definition.acceptedMinimumTyreClearanceMetres(),
				kinematic_definition.tyreTessellationToleranceMetres());
			tyre_pose_count += sweep.poseSamples().size();
			native_tyre_validation_passed = native_tyre_validation_passed &&
				sweep.validationReport().passed();
			tyre_sweeps.push_back(std::move(sweep));
		}
		native_tyre_validation_passed = native_tyre_validation_passed &&
			tyre_pose_count == 36u;
		VehicleTyreSweepSet tyre_sweep_set(std::move(tyre_sweeps));

		const VehicleClosureSweepSet raw_closure_sweeps =
			closure_service_.createClosureSweepSet(kinematic_definition);
		bool closure_binding_and_motion_passed =
			raw_closure_sweeps.sweeps().size() == 6u &&
			semantic_bindings.closureMeshes().size() == 6u;
		std::size_t closure_pose_count = 0u;
		std::vector<VehicleClosureSweep> validated_closure_sweeps;
		validated_closure_sweeps.reserve(raw_closure_sweeps.sweeps().size());
		bool native_closure_validation_passed = true;
		const McsMv22NativePartitionGeometry &partition_geometry =
			evidence_geometry.progen3dPartitionGeometry();
		for (const VehicleClosureSweep &raw_sweep : raw_closure_sweeps.sweeps()) {
			closure_pose_count += raw_sweep.poseSamples().size();
			closure_binding_and_motion_passed = closure_binding_and_motion_passed &&
				raw_sweep.surfaceBinding().sourceFaceCount() > 0u &&
				raw_sweep.poseSamples().size() == 7u;
			for (const VehicleClosurePoseSample &sample : raw_sweep.poseSamples()) {
				closure_binding_and_motion_passed =
					closure_binding_and_motion_passed && isRigid(sample.sourceTransform());
			}
			const auto closure_mesh = partition_geometry.closureMeshes().find(
				raw_sweep.surfaceBinding().closureIdentifier());
			if (closure_mesh == partition_geometry.closureMeshes().end()) {
				throw std::runtime_error(
					"MCSMv2.2 native closure evidence mesh is missing: " +
					raw_sweep.surfaceBinding().closureIdentifier());
			}
			VehicleClosureSweep validated_sweep =
				closure_validation_service_.validateAndBuildSweep(
					raw_sweep, closure_mesh->second, partition_geometry.fixedBodyMesh(),
					source_release.toProgen3dMatrix());
			native_closure_validation_passed =
				native_closure_validation_passed && validated_sweep.passed();
			validated_closure_sweeps.push_back(std::move(validated_sweep));
		}
		closure_binding_and_motion_passed =
			closure_binding_and_motion_passed && closure_pose_count == 42u;
		VehicleClosureSweepSet closure_sweep_set(
			std::move(validated_closure_sweeps));

		const double parent_closure_state = 0.55;
		const VehicleGlassMotionSet raw_glass_motions =
			closure_service_.createGlassMotionSet(
				kinematic_definition, parent_closure_state);
		bool glass_binding_and_motion_passed =
			raw_glass_motions.motions().size() == 4u &&
			semantic_bindings.glassMeshes().size() == 4u;
		std::size_t glass_pose_count = 0u;
		std::vector<VehicleGlassMotion> validated_glass_motions;
		validated_glass_motions.reserve(raw_glass_motions.motions().size());
		bool native_glass_validation_passed = true;
		const VehicleClosureMotionEvaluationService closure_motion_service;
		for (const VehicleGlassMotion &raw_motion : raw_glass_motions.motions()) {
			glass_pose_count += raw_motion.poseSamples().size();
			glass_binding_and_motion_passed = glass_binding_and_motion_passed &&
				raw_motion.surfaceBinding().sourceFaceCount() > 0u &&
				raw_motion.poseSamples().size() == 6u;
			for (const VehicleGlassPoseSample &sample : raw_motion.poseSamples()) {
				glass_binding_and_motion_passed =
					glass_binding_and_motion_passed &&
					isRigid(sample.worldSourceTransform());
			}
			const VehicleClosureSweep *parent_sweep = findClosureSweep(
				raw_closure_sweeps,
				raw_motion.surfaceBinding().parentClosureIdentifier());
			if (parent_sweep == nullptr) {
				throw std::runtime_error(
					"MCSMv2.2 glass parent closure sweep is missing: " +
					raw_motion.surfaceBinding().parentClosureIdentifier());
			}
			const glm::dmat4 parent_source_transform = closure_motion_service
				.evaluateClosurePose(
					parent_sweep->surfaceBinding().joint(), parent_closure_state)
				.sourceTransform();
			const auto glass_mesh = partition_geometry.glassMeshes().find(
				raw_motion.surfaceBinding().apertureDomainIdentifier());
			const auto parent_closure_mesh = partition_geometry.closureMeshes().find(
				raw_motion.surfaceBinding().parentClosureIdentifier());
			if (glass_mesh == partition_geometry.glassMeshes().end() ||
			    parent_closure_mesh == partition_geometry.closureMeshes().end()) {
				throw std::runtime_error(
					"MCSMv2.2 native glass or parent-closure evidence mesh is missing.");
			}
			VehicleGlassMotion validated_motion =
				glass_validation_service_.validateAndBuildMotion(
					raw_motion, glass_mesh->second, parent_closure_mesh->second,
					partition_geometry.fixedBodyMesh(), parent_source_transform,
					source_release.toProgen3dMatrix());
			native_glass_validation_passed =
				native_glass_validation_passed && validated_motion.passed();
			validated_glass_motions.push_back(std::move(validated_motion));
		}
		glass_binding_and_motion_passed =
			glass_binding_and_motion_passed && glass_pose_count == 24u;
		VehicleGlassMotionSet glass_motion_set(std::move(validated_glass_motions));

		const bool hardpoint_validation_passed =
			corners.size() == 4u && kinematic_definition.hardpoints().size() == 32u;
		const bool source_release_passed =
			kinematic_definition.sourceReleaseGatePass() &&
			kinematic_definition.assuranceLevel() == "V3";
		McsMv22KinematicAssuranceReport assurance = assurance_service_.evaluate(
			kinematic_definition.identifier(), source_release_passed,
			semantic_geometry->assuranceV2Passed(), hardpoint_validation_passed,
			tyre_topology_passed && wheel_pose_parity_passed,
			native_tyre_validation_passed, closure_binding_and_motion_passed,
			native_closure_validation_passed, glass_binding_and_motion_passed,
			native_glass_validation_passed);
		if (!assurance.achievedV3()) {
			return GeneratedMcsMv22KinematicGeometryResult(
				std::nullopt, joinDiagnostics(assurance));
		}

		return GeneratedMcsMv22KinematicGeometryResult(
			McsMv22KinematicGeometry(
				std::move(semantic_geometry), corners, std::move(tyre_sweep_set),
				std::move(closure_sweep_set), std::move(glass_motion_set),
				std::move(assurance)),
			std::string());
	}
	catch (const std::exception &error) {
		return GeneratedMcsMv22KinematicGeometryResult(std::nullopt, error.what());
	}
}
