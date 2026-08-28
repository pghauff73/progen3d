#include "vehicle/service/VehicleFittingDeterministicHashService.h"

#include <cstdint>
#include <string>
#include <variant>

namespace {

class VehicleFittingHashAccumulator
{
public:
	void appendByte(std::uint8_t value)
	{
		hash_ ^= value;
		hash_ *= 1099511628211ULL;
	}

	template <typename Value>
	void appendValue(const Value &value)
	{
		const auto *bytes = reinterpret_cast<const std::uint8_t *>(&value);
		for (std::size_t index = 0u; index < sizeof(Value); ++index) {
			appendByte(bytes[index]);
		}
	}

	void appendString(const std::string &value)
	{
		for (unsigned char character : value) appendByte(character);
		appendByte(0xffu);
	}

	void appendVector2(const glm::vec2 &value)
	{
		appendValue(value.x);
		appendValue(value.y);
	}

	void appendVector3(const glm::vec3 &value)
	{
		appendValue(value.x);
		appendValue(value.y);
		appendValue(value.z);
	}

	std::uint64_t value() const { return hash_; }

private:
	std::uint64_t hash_ = 14695981039346656037ULL;
};

void append_curve(VehicleFittingHashAccumulator *hash, const Curve3D &curve)
{
	hash->appendValue(static_cast<int>(curve.type()));
	hash->appendString(curve.identifier());
	for (const glm::vec3 &point : curve.controlPoints()) hash->appendVector3(point);
}

void append_bounds(VehicleFittingHashAccumulator *hash, const AxisAlignedBounds &bounds)
{
	hash->appendValue(bounds.valid);
	hash->appendVector3(bounds.min);
	hash->appendVector3(bounds.max);
}

} // namespace

std::uint64_t VehicleFittingDeterministicHashService::calculate(
	const VehicleFittingArchitecture &architecture) const
{
	VehicleFittingHashAccumulator hash;
	hash.appendString("MVPv2-Hatchback-FittingArchitecture-P0");
	hash.appendString(architecture.identifier());
	hash.appendString(architecture.multiViewEnvelope().identifier());
	hash.appendValue(architecture.multiViewEnvelope().projectionTolerance());
	for (const VehicleViewEnvelope &view :
	     architecture.multiViewEnvelope().viewEnvelopes()) {
		hash.appendString(view.identifier());
		hash.appendValue(static_cast<int>(view.view()));
		hash.appendString(view.cameraIdentifier());
		hash.appendValue(view.fittingWeight());
		for (const ViewSilhouetteSample &sample : view.silhouetteSamples()) {
			hash.appendString(sample.semanticName());
			hash.appendVector2(sample.imagePoint());
		}
	}
	for (const VehicleCrossSection &section : architecture.crossSections()) {
		hash.appendString(section.identifier());
		hash.appendValue(section.stationZ());
		for (const VehicleSectionLandmark &landmark : section.landmarks()) {
			hash.appendValue(static_cast<int>(landmark.role()));
			hash.appendVector3(landmark.point());
		}
	}
	for (const SurfaceLandmark &landmark : architecture.surfaceLandmarks()) {
		hash.appendString(landmark.identifier());
		hash.appendVector3(landmark.fittedPoint());
		hash.appendValue(landmark.confidence());
		hash.appendValue(static_cast<int>(landmark.evidenceClassification()));
		for (const SurfaceLandmarkObservation &observation : landmark.observations()) {
			hash.appendValue(static_cast<int>(observation.view()));
			hash.appendVector2(observation.imagePoint());
			hash.appendValue(observation.weight());
		}
	}
	for (const VehicleCharacterCurve &curve :
	     architecture.characterCurveNetwork().curves()) {
		hash.appendString(curve.identifier());
		hash.appendValue(static_cast<int>(curve.role()));
		append_curve(&hash, curve.curve());
		for (const std::string &landmark : curve.landmarkIdentifiers()) {
			hash.appendString(landmark);
		}
	}
	for (const CharacterCurveRelationship &relationship :
	     architecture.characterCurveNetwork().relationships()) {
		hash.appendString(relationship.sourceCurveIdentifier());
		hash.appendString(relationship.targetCurveIdentifier());
		hash.appendValue(static_cast<int>(relationship.relationshipType()));
		hash.appendValue(relationship.tolerance());
	}
	for (const ProjectionConstrainedPatch &patch :
	     architecture.surfacePatchGraph().patches()) {
		hash.appendString(patch.initialPatch().identifier());
		for (const std::string &boundary :
		     patch.initialPatch().boundaryCurveIdentifiers()) {
			hash.appendString(boundary);
		}
		for (const PatchProjectionConstraint &projection :
		     patch.projectionConstraints()) {
			hash.appendValue(static_cast<int>(projection.view()));
			hash.appendString(projection.targetIdentifier());
			hash.appendValue(projection.weight());
			hash.appendValue(projection.residual());
		}
		hash.appendValue(patch.continuityWeight());
		hash.appendValue(patch.complexityWeight());
	}
	for (const SurfaceEdgeRelationship &relationship :
	     architecture.surfacePatchGraph().edgeRelationships()) {
		hash.appendString(relationship.sourcePatchIdentifier());
		hash.appendString(relationship.sourceEdgeIdentifier());
		hash.appendString(relationship.targetPatchIdentifier());
		hash.appendString(relationship.targetEdgeIdentifier());
		hash.appendValue(static_cast<int>(relationship.relationshipType()));
		hash.appendValue(relationship.tolerance());
	}
	for (const PanelSeamLoop &seam : architecture.panelSeamLoops()) {
		hash.appendString(seam.identifier());
		hash.appendString(seam.hostPatchIdentifier());
		append_curve(&hash, seam.closedBoundary());
		hash.appendValue(seam.gapWidth());
		hash.appendValue(seam.gapDepth());
	}
	for (const ExtractedSurfacePanel &panel : architecture.extractedPanels()) {
		hash.appendString(panel.identifier());
		hash.appendString(panel.sourcePatchIdentifier());
		hash.appendString(panel.seamLoopIdentifier());
	}
	for (const Aperture &aperture : architecture.apertures()) {
		hash.appendString(aperture.identifier());
		hash.appendString(aperture.hostStructureIdentifier());
		hash.appendString(aperture.outerBoundaryIdentifier());
		hash.appendString(aperture.innerBoundaryIdentifier());
		hash.appendValue(aperture.flangeDepth());
		hash.appendString(aperture.sealSeatIdentifier());
	}
	for (const ClosureAssembly &closure : architecture.closureAssemblies()) {
		hash.appendString(closure.identifier());
		hash.appendValue(static_cast<int>(closure.closureType()));
		hash.appendString(closure.hostObjectIdentifier());
		hash.appendString(closure.apertureIdentifier());
		hash.appendString(closure.outerPanelIdentifier());
		hash.appendString(closure.innerPanelIdentifier());
		hash.appendValue(closure.state());
		if (std::holds_alternative<HingePair>(closure.kinematicRelationship())) {
			const HingePair &hinge = std::get<HingePair>(closure.kinematicRelationship());
			hash.appendString("HingePair");
			hash.appendString(hinge.identifier());
			hash.appendVector3(hinge.lowerHingePoint());
			hash.appendVector3(hinge.upperHingePoint());
			hash.appendValue(hinge.maximumAngleDegrees());
		}
		else {
			const FourBarJoint &joint = std::get<FourBarJoint>(closure.kinematicRelationship());
			hash.appendString("FourBarJoint");
			hash.appendString(joint.identifier());
			hash.appendVector3(joint.bodyMountA());
			hash.appendVector3(joint.bodyMountB());
			hash.appendVector3(joint.closureMountA());
			hash.appendVector3(joint.closureMountB());
			hash.appendValue(joint.maximumAngleDegrees());
			hash.appendVector3(joint.openTranslation());
		}
		append_bounds(&hash, closure.sweptVolume().sourceBounds());
		append_bounds(&hash, closure.sweptVolume().sweptBounds());
		hash.appendValue(closure.sweptVolume().sampleCount());
	}
	for (const DropGlassAssembly &glass : architecture.dropGlassAssemblies()) {
		hash.appendString(glass.identifier());
		hash.appendString(glass.parentClosureIdentifier());
		hash.appendString(glass.glassPanelIdentifier());
		hash.appendValue(glass.state());
		append_curve(&hash, glass.guideRailJoint().frontRail());
		append_curve(&hash, glass.guideRailJoint().rearRail());
		hash.appendVector3(glass.guideRailJoint().frontFollower());
		hash.appendVector3(glass.guideRailJoint().rearFollower());
		append_bounds(&hash, glass.doorInnerVolume());
	}
	for (float state : architecture.closureState().orderedValues()) {
		hash.appendValue(state);
	}
	return hash.value();
}
