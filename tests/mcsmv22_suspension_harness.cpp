#include "geometry/service/MeshTopologyAnalyzer.h"
#include "vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv22SuspensionKinematicEvaluationService.h"
#include "vehicle/model/VehicleTyreMeshSpecification.h"
#include "vehicle/service/VehicleReferenceFrameTransformationService.h"
#include "vehicle/service/VehicleTyreMeshGenerationService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <string>

namespace {

bool require(bool condition, const std::string &message)
{
	if (!condition) std::cerr << "FAIL: " << message << '\n';
	return condition;
}

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

std::string mirroredIdentifier(const std::string &identifier)
{
	const std::string left_token = "_left_";
	const std::size_t position = identifier.find(left_token);
	if (position == std::string::npos) return {};
	std::string mirrored = identifier;
	mirrored.replace(position, left_token.size(), "_right_");
	return mirrored;
}

} // namespace

int main()
{
	const McsMv22KinematicFamilyDefinition family =
		McsMv22KinematicCatalogFactory().createFamilyDefinition();
	const McsMv22SuspensionKinematicEvaluationService suspension_service;
	const VehicleReferenceFrameTransformationService frame_service;
	const VehicleTyreMeshGenerationService tyre_service;
	bool passed = true;

	passed &= require(
		closeVector(
			frame_service.transformPoint(
				glm::dvec3(1.0, 2.0, 3.0),
				family.sourceRelease().toProgen3dMatrix()),
			glm::dvec3(2.0, 3.0, 1.0)),
		"source X/Y/Z must map to ProGen Z/X/Y");

	for (const McsMv22KinematicVariantDefinition &variant : family.variants()) {
		const std::vector<VehicleWheelPoseEvaluation> evaluations =
			suspension_service.evaluateAcceptedPoseGrid(variant);
		passed &= require(
			evaluations.size() == variant.acceptedWheelPoses().size(),
			variant.identifier() + " evaluation count must match accepted evidence");

		std::size_t front_count = 0u;
		std::size_t rear_count = 0u;
		for (std::size_t pose_index = 0u; pose_index < evaluations.size();
		     ++pose_index) {
			const McsMv22WheelPoseEvidence &accepted =
				variant.acceptedWheelPoses()[pose_index];
			const VehicleWheelPoseEvaluation &evaluated = evaluations[pose_index];
			const bool front = accepted.wheelIdentifier().find("front_") == 0u;
			front ? ++front_count : ++rear_count;
			passed &= require(
				close(evaluated.steeringDegrees(), accepted.steerDegrees()),
				variant.identifier() + " steering parity failed");
			passed &= require(
				close(evaluated.suspensionTravelMetres(), accepted.travelMetres()),
				variant.identifier() + " travel parity failed");
			passed &= require(
				close(evaluated.camberDegrees(), accepted.camberDegrees()),
				variant.identifier() + " camber parity failed");
			passed &= require(
				close(evaluated.toeDegrees(), accepted.toeDegrees()),
				variant.identifier() + " toe parity failed");
			passed &= require(
				closeVector(evaluated.sourceCentre(), accepted.sourceCentre()),
				variant.identifier() + " source centre parity failed");
			passed &= require(
				closeMatrix(evaluated.sourceTransform(), accepted.sourceTransform()),
				variant.identifier() + " source transform parity failed");
			passed &= require(
				close(glm::determinant(glm::dmat3(evaluated.sourceTransform())), 1.0,
				      1.0e-10),
				variant.identifier() + " wheel rotation determinant must be one");

			const glm::dvec3 target_centre = frame_service.transformPoint(
				evaluated.sourceCentre(), family.sourceRelease().toProgen3dMatrix());
			const glm::dmat4 target_pose = frame_service.transformRigidBodyPose(
				evaluated.sourceTransform(),
				family.sourceRelease().toProgen3dMatrix());
			passed &= require(
				closeVector(glm::dvec3(target_pose[3]), target_centre),
				variant.identifier() + " transformed pose translation must match centre");
			passed &= require(
				target_centre.y >= variant.wheelRadiusMetres() +
					variant.minimumTravelMetres() - 1.0e-12,
				variant.identifier() + " transformed wheel centre must remain grounded");
		}
		passed &= require(front_count == 30u, variant.identifier() + " must evaluate 30 front poses");
		passed &= require(rear_count == 6u, variant.identifier() + " must evaluate six rear poses");

		std::map<std::string, const McsMv22SuspensionHardpointDefinition *> hardpoints;
		for (const McsMv22SuspensionHardpointDefinition &hardpoint :
		     variant.hardpoints()) {
			hardpoints.emplace(hardpoint.identifier(), &hardpoint);
		}
		for (const McsMv22SuspensionHardpointDefinition &left :
		     variant.hardpoints()) {
			const std::string mirrored_identifier = mirroredIdentifier(left.identifier());
			if (mirrored_identifier.empty()) continue;
			const auto right = hardpoints.find(mirrored_identifier);
			passed &= require(
				right != hardpoints.end(),
				variant.identifier() + " hardpoint mirror must exist");
			if (right == hardpoints.end()) continue;
			passed &= require(
				close(left.sourcePosition().x, right->second->sourcePosition().x,
				      1.0e-9) &&
				close(left.sourcePosition().y, -right->second->sourcePosition().y,
				      1.0e-9) &&
				close(left.sourcePosition().z, right->second->sourcePosition().z,
				      1.0e-9),
				variant.identifier() + " hardpoint mirror coordinates must match");
		}

		const VehicleTyreMeshSpecification tyre_specification(
			variant.wheelRadiusMetres(), variant.wheelWidthMetres());
		const GeneratedPrimitiveMesh tyre =
			tyre_service.generateSourceFrameTyre(tyre_specification);
		passed &= require(
			tyre.mesh()->vertices.size() == 560u,
			variant.identifier() + " tyre must contain 560 vertices");
		passed &= require(
			tyre.mesh()->faces.size() == 1120u,
			variant.identifier() + " tyre must contain 1120 faces");
		passed &= require(
			tyre.faceSurfaceTags().size() == tyre.mesh()->faces.size(),
			variant.identifier() + " tyre must tag every face");
		passed &= require(
			tyre.mesh()->normals.size() == tyre.mesh()->faces.size(),
			variant.identifier() + " tyre must calculate one normal per face");
		const MeshTopologyReport topology =
			MeshTopologyAnalyzer().analyze(*tyre.mesh(), tyre.faceSurfaceTags());
		passed &= require(
			topology.isWatertight(), variant.identifier() + " tyre must be watertight");
		double maximum_radius = 0.0;
		double maximum_width = 0.0;
		for (const glm::vec3 &vertex : tyre.mesh()->vertices) {
			maximum_radius = std::max(
				maximum_radius,
				std::sqrt(static_cast<double>(vertex.x * vertex.x +
				                                  vertex.z * vertex.z)));
			maximum_width = std::max(maximum_width, 2.0 * std::abs(vertex.y));
		}
		passed &= require(
			close(maximum_radius, variant.wheelRadiusMetres(), 1.0e-6),
			variant.identifier() + " tyre outer radius must match source");
		passed &= require(
			maximum_width <= variant.wheelWidthMetres() + 1.0e-6,
			variant.identifier() + " tyre width must not exceed source");
	}

	if (!passed) return 1;
	std::cout << "MCSMv2.2 suspension and tyre checks passed.\n";
	return 0;
}
