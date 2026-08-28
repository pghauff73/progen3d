#include "vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>
#include <set>
#include <string>

namespace {

bool require(bool condition, const std::string &message)
{
	if (!condition) std::cerr << "FAIL: " << message << '\n';
	return condition;
}

bool finiteMatrix(const glm::dmat4 &matrix)
{
	for (std::size_t column = 0u; column < 4u; ++column) {
		for (std::size_t row = 0u; row < 4u; ++row) {
			if (!std::isfinite(matrix[column][row])) return false;
		}
	}
	return true;
}

} // namespace

int main()
{
	const McsMv22KinematicFamilyDefinition family =
		McsMv22KinematicCatalogFactory().createFamilyDefinition();
	bool passed = true;
	passed &= require(family.sourceRelease().version() == "2.2.0", "source version must be 2.2.0");
	passed &= require(
		family.sourceRelease().releaseManifestSha256() ==
			"9053996bc0e8ba0017ad8eeffb616ab801b0b709dbe0fef93708daa93518882b",
		"source manifest hash must be exact");
	passed &= require(family.sourceRelease().signedArtifactCount() == 109u, "signed artifact count must be 109");
	passed &= require(family.sourceRelease().signedTotalSizeBytes() == 103068076u, "signed byte count must be exact");
	passed &= require(finiteMatrix(family.sourceRelease().toProgen3dMatrix()), "frame adapter must be finite");
	passed &= require(family.variants().size() == 4u, "family must contain four variants");

	const std::string expected_identifiers[] = {"reference", "track", "aero", "crossover"};
	for (std::size_t index = 0u; index < family.variants().size(); ++index) {
		const McsMv22KinematicVariantDefinition &variant = family.variants()[index];
		passed &= require(variant.identifier() == expected_identifiers[index], "variant order must be deterministic");
		passed &= require(variant.hardpoints().size() == 32u, variant.identifier() + " must contain 32 hardpoints");
		passed &= require(variant.acceptedWheelPoses().size() == 36u, variant.identifier() + " must contain 36 poses");
		passed &= require(variant.hinges().size() == 6u, variant.identifier() + " must contain six hinges");
		passed &= require(variant.glassSystems().size() == 4u, variant.identifier() + " must contain four glass systems");
		passed &= require(variant.panelOwners().size() == 17u, variant.identifier() + " must contain 17 panel owners");
		passed &= require(variant.apertureOwners().size() == 6u, variant.identifier() + " must contain six aperture owners");
		passed &= require(variant.wheelRadiusMetres() > 0.0, variant.identifier() + " wheel radius must be positive");
		passed &= require(variant.wheelWidthMetres() > 0.0, variant.identifier() + " wheel width must be positive");
		passed &= require(variant.minimumSteerDegrees() < variant.maximumSteerDegrees(), variant.identifier() + " steer interval must be ordered");
		passed &= require(variant.minimumTravelMetres() < variant.maximumTravelMetres(), variant.identifier() + " travel interval must be ordered");
		passed &= require(variant.sourceReleaseGatePass(), variant.identifier() + " release gate must pass");
		passed &= require(variant.assuranceLevel() == "V3", variant.identifier() + " assurance must be V3");
		passed &= require(
			variant.acceptedMinimumTyreClearanceMetres() +
				variant.tyreTessellationToleranceMetres() >=
				variant.declaredTyreClearanceMetres(),
			variant.identifier() +
				" accepted tyre clearance must meet the declaration within tessellation tolerance");

		std::set<std::string> closure_identifiers;
		for (const McsMv22ClosureHingeDefinition &hinge : variant.hinges()) {
			closure_identifiers.insert(hinge.closureIdentifier());
			passed &= require(glm::length(hinge.sourceAxis()) > 0.0, variant.identifier() + " hinge axis must be nonzero");
		}
		passed &= require(closure_identifiers.size() == 6u, variant.identifier() + " closure identifiers must be unique");

		std::set<std::string> wheel_identifiers;
		std::size_t front_pose_count = 0u;
		std::size_t rear_pose_count = 0u;
		for (const McsMv22WheelPoseEvidence &pose : variant.acceptedWheelPoses()) {
			wheel_identifiers.insert(pose.wheelIdentifier());
			if (pose.wheelIdentifier().find("front_") == 0u) ++front_pose_count;
			if (pose.wheelIdentifier().find("rear_") == 0u) ++rear_pose_count;
			passed &= require(finiteMatrix(pose.sourceTransform()), variant.identifier() + " pose transform must be finite");
		}
		passed &= require(wheel_identifiers.size() == 4u, variant.identifier() + " must contain four wheel identifiers");
		passed &= require(front_pose_count == 30u, variant.identifier() + " must contain 30 front poses");
		passed &= require(rear_pose_count == 6u, variant.identifier() + " must contain six rear poses");
	}

	if (!passed) return 1;
	std::cout << "MCSMv2.2 catalog checks passed.\n";
	return 0;
}
