#include "vehicle/service/VehicleMvp25DeterministicHashService.h"

#include <cstdint>
#include <string>

namespace {

class VehicleMvp25HashAccumulator
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

	void appendMatrix4(const glm::mat4 &value)
	{
		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) appendValue(value[column][row]);
		}
	}

	std::uint64_t value() const { return hash_; }

private:
	std::uint64_t hash_ = 14695981039346656037ULL;
};

void append_bounds(VehicleMvp25HashAccumulator *hash, const AxisAlignedBounds &bounds)
{
	hash->appendValue(bounds.valid);
	hash->appendVector3(bounds.min);
	hash->appendVector3(bounds.max);
}

void append_curve(VehicleMvp25HashAccumulator *hash, const Curve3D &curve)
{
	hash->appendString(curve.identifier());
	hash->appendValue(static_cast<int>(curve.type()));
	for (const glm::vec3 &point : curve.controlPoints()) hash->appendVector3(point);
}

void append_line_observation(
	VehicleMvp25HashAccumulator *hash,
	const CharacterLineObservation &observation)
{
	hash->appendString(observation.identifier());
	hash->appendString(observation.cameraIdentifier());
	for (const glm::vec2 &point : observation.imagePoints()) hash->appendVector2(point);
	hash->appendValue(observation.uncertainty());
	hash->appendValue(observation.confidence());
}

} // namespace

std::uint64_t VehicleMvp25DeterministicHashService::calculate(
	const VehicleMvp25Architecture &architecture) const
{
	VehicleMvp25HashAccumulator hash;
	hash.appendString("MVPv2.5-EvidenceConstrainedAutomotiveGeometry-v1");
	hash.appendString(architecture.identifier());

	const VehicleReferenceFrame &frame = architecture.referenceFrame();
	hash.appendValue(static_cast<int>(frame.convention()));
	hash.appendVector3(frame.origin());
	hash.appendVector3(frame.longitudinalAxis());
	hash.appendVector3(frame.lateralAxis());
	hash.appendVector3(frame.verticalAxis());
	hash.appendValue(frame.longitudinalZeroPlane());
	hash.appendValue(frame.lateralZeroPlane());
	hash.appendValue(frame.verticalZeroPlane());
	hash.appendValue(frame.groundPlane());
	for (const VehicleFiducialDatum &datum : frame.fiducialDatums()) {
		hash.appendString(datum.identifier());
		hash.appendVector3(datum.point());
		hash.appendVector3(datum.direction());
	}

	const VehiclePackageEvidence &package = architecture.packageEvidence();
	hash.appendValue(package.overallLength());
	hash.appendValue(package.overallWidth());
	hash.appendValue(package.overallHeight());
	hash.appendValue(package.wheelbase());
	hash.appendValue(package.frontTrack());
	hash.appendValue(package.rearTrack());
	hash.appendValue(package.frontOverhang());
	hash.appendValue(package.rearOverhang());
	hash.appendVector3(package.frontAxleCentre());
	hash.appendVector3(package.rearAxleCentre());
	hash.appendValue(static_cast<int>(package.drive()));
	hash.appendValue(package.tolerance());
	for (const OccupantPackage &occupant : architecture.occupantPackages()) {
		hash.appendString(occupant.identifier());
		hash.appendVector3(occupant.hPoint());
		hash.appendVector3(occupant.eyePoint());
		hash.appendVector3(occupant.heelPoint());
		append_bounds(&hash, occupant.kneeEnvelope());
		append_bounds(&hash, occupant.headEnvelope());
		hash.appendVector3(occupant.torsoDirection());
		append_bounds(&hash, occupant.reachEnvelope());
	}

	for (const VehicleViewObservation &view : architecture.observations().observations()) {
		hash.appendString(view.identifier());
		hash.appendString(view.imageIdentifier());
		const VehicleCameraModel &camera = view.camera();
		hash.appendString(camera.identifier());
		hash.appendValue(static_cast<int>(camera.view()));
		hash.appendValue(static_cast<int>(camera.projection()));
		hash.appendMatrix4(camera.worldToCamera());
		hash.appendVector2(camera.principalPoint());
		hash.appendValue(camera.focalLength());
		hash.appendValue(camera.orthographicScale());
		hash.appendVector2(camera.imageDimensions());
		for (const glm::vec2 &point : view.silhouette()) hash.appendVector2(point);
		for (const std::string &identifier : view.landmarkIdentifiers()) {
			hash.appendString(identifier);
		}
		for (const CharacterLineObservation &observation : view.characterLines()) {
			append_line_observation(&hash, observation);
		}
		for (const CharacterLineObservation &observation : view.panelLines()) {
			append_line_observation(&hash, observation);
		}
		for (const CharacterLineObservation &observation : view.glazingLines()) {
			append_line_observation(&hash, observation);
		}
		for (const CharacterLineObservation &observation : view.wheelEllipses()) {
			append_line_observation(&hash, observation);
		}
	}

	const AutomotiveWireframe &wireframe = architecture.wireframe();
	hash.appendString(wireframe.identifier());
	hash.appendString(wireframe.shapePrior().vehicleClass());
	hash.appendString(wireframe.shapePrior().topologyIdentifier());
	hash.appendValue(wireframe.shapePrior().weight());
	hash.appendValue(static_cast<int>(wireframe.shapePrior().strength()));
	for (const VehicleLandmark &landmark : wireframe.landmarks()) {
		hash.appendString(landmark.identifier());
		hash.appendVector3(landmark.position());
		hash.appendString(landmark.symmetryPartnerIdentifier());
		hash.appendString(landmark.topologyRole());
		hash.appendValue(static_cast<int>(landmark.evidenceClassification()));
		hash.appendValue(static_cast<int>(landmark.constraintStrength()));
		for (const VehicleLandmarkObservation &observation : landmark.observations()) {
			hash.appendString(observation.cameraIdentifier());
			hash.appendVector2(observation.imagePoint());
			hash.appendValue(observation.uncertainty());
			hash.appendValue(observation.confidence());
		}
	}
	for (const AutomotiveWireframeEdge &edge : wireframe.edges()) {
		hash.appendString(edge.identifier());
		hash.appendString(edge.firstLandmarkIdentifier());
		hash.appendString(edge.secondLandmarkIdentifier());
		hash.appendValue(static_cast<int>(edge.role()));
	}
	for (const SilhouetteConstraint &constraint : architecture.silhouetteConstraints()) {
		hash.appendString(constraint.identifier());
		hash.appendString(constraint.cameraIdentifier());
		for (const glm::vec3 &point : constraint.modelBoundaryPoints()) {
			hash.appendVector3(point);
		}
		for (const glm::vec2 &point : constraint.observedSilhouette()) {
			hash.appendVector2(point);
		}
		hash.appendValue(constraint.weight());
		hash.appendValue(static_cast<int>(constraint.strength()));
		hash.appendValue(constraint.tolerance());
	}

	for (const AutomotiveCharacterCurve &curve : architecture.classASurfaces().curves()) {
		hash.appendString(curve.identifier());
		hash.appendString(curve.semanticRole());
		hash.appendValue(curve.degree());
		for (const glm::vec3 &point : curve.controlPoints()) hash.appendVector3(point);
		for (const std::string &identifier : curve.observationIdentifiers()) {
			hash.appendString(identifier);
		}
		for (const std::string &identifier : curve.continuityTargetIdentifiers()) {
			hash.appendString(identifier);
		}
	}
	for (const ClassASurfacePatch &patch : architecture.classASurfaces().patches()) {
		hash.appendString(patch.identifier());
		for (const std::string &identifier : patch.boundaryCurveIdentifiers()) {
			hash.appendString(identifier);
		}
		for (const std::string &identifier : patch.internalGuideIdentifiers()) {
			hash.appendString(identifier);
		}
		hash.appendValue(patch.degreeU());
		hash.appendValue(patch.degreeV());
		hash.appendValue(patch.spanCountU());
		hash.appendValue(patch.spanCountV());
		hash.appendValue(patch.controlPointColumns());
		hash.appendValue(patch.controlPointRows());
		for (const glm::vec3 &point : patch.controlPoints()) hash.appendVector3(point);
	}
	for (const PatchConnection &connection : architecture.classASurfaces().connections()) {
		hash.appendString(connection.identifier());
		hash.appendString(connection.firstPatchIdentifier());
		hash.appendString(connection.firstEdgeIdentifier());
		hash.appendString(connection.secondPatchIdentifier());
		hash.appendString(connection.secondEdgeIdentifier());
		hash.appendValue(static_cast<int>(connection.continuity()));
		hash.appendValue(connection.tolerance());
	}

	for (const AutomotivePanelGap &gap : architecture.panelGaps()) {
		hash.appendString(gap.identifier());
		append_curve(&hash, gap.seamCurve());
		for (const AutomotiveScalarStation &station : gap.gapWidthStations()) {
			hash.appendValue(station.parameter());
			hash.appendValue(station.value());
		}
		for (const AutomotiveScalarStation &station : gap.primaryRadiusStations()) {
			hash.appendValue(station.parameter());
			hash.appendValue(station.value());
		}
		for (const AutomotiveScalarStation &station : gap.secondaryRadiusStations()) {
			hash.appendValue(station.parameter());
			hash.appendValue(station.value());
		}
		hash.appendString(gap.primaryFlange().identifier());
		hash.appendString(gap.primaryFlange().contactCurveIdentifier());
		hash.appendValue(gap.primaryFlange().length());
		hash.appendValue(gap.primaryFlange().angleDegrees());
		hash.appendString(gap.secondaryFlange().identifier());
		hash.appendString(gap.secondaryFlange().contactCurveIdentifier());
		hash.appendValue(gap.secondaryFlange().length());
		hash.appendValue(gap.secondaryFlange().angleDegrees());
		hash.appendValue(static_cast<int>(gap.primaryContinuity()));
		hash.appendValue(static_cast<int>(gap.secondaryContinuity()));
		hash.appendValue(gap.hasCloseout());
	}
	for (const RolledEdge &edge : architecture.rolledEdges()) {
		hash.appendString(edge.identifier());
		append_curve(&hash, edge.contactCurve());
		hash.appendValue(edge.radius());
		hash.appendValue(edge.flangeLength());
		hash.appendValue(edge.flangeAngleDegrees());
		hash.appendValue(static_cast<int>(edge.continuity()));
	}
	for (const BodySideAperture &aperture : architecture.bodySideApertures()) {
		hash.appendString(aperture.identifier());
		hash.appendString(aperture.hingePillarIdentifier());
		hash.appendString(aperture.aPillarIdentifier());
		hash.appendString(aperture.roofRailIdentifier());
		hash.appendString(aperture.bPillarIdentifier());
		hash.appendString(aperture.rockerIdentifier());
		hash.appendString(aperture.cPillarIdentifier());
		hash.appendString(aperture.doglegIdentifier());
		append_curve(&hash, aperture.bLine());
		hash.appendString(aperture.jSurfaceIdentifier());
		hash.appendString(aperture.glassSurfaceIdentifier());
		hash.appendString(aperture.flangeIdentifier());
		hash.appendString(aperture.sealSeatIdentifier());
	}
	for (const DoorEgressSurface &surface : architecture.doorEgressSurfaces()) {
		hash.appendString(surface.identifier());
		hash.appendString(surface.bLineIdentifier());
		hash.appendString(surface.sideGlassIdentifier());
		hash.appendString(surface.jSurfaceIdentifier());
		hash.appendString(surface.beltLineIdentifier());
		hash.appendValue(surface.upperOffset());
		hash.appendValue(surface.lowerOffset());
	}
	for (const AutomotiveClosure &closure : architecture.closures()) {
		hash.appendString(closure.identifier());
		hash.appendValue(static_cast<int>(closure.type()));
		hash.appendString(closure.outerClassAPatchIdentifier());
		hash.appendString(closure.apertureIdentifier());
		hash.appendString(closure.jSurfaceIdentifier());
		hash.appendString(closure.sideGlassIdentifier());
		const ClosureHingeStudy &hinge = closure.hingeStudy();
		hash.appendString(hinge.identifier());
		hash.appendVector3(hinge.upperHinge());
		hash.appendVector3(hinge.lowerHinge());
		hash.appendValue(hinge.minimumOpeningDegrees());
		hash.appendValue(hinge.maximumOpeningDegrees());
		for (const glm::vec2 &rise : hinge.riseCurve()) hash.appendVector2(rise);
		append_bounds(&hash, hinge.sourceBounds());
		append_bounds(&hash, hinge.sweptBounds());
		for (const std::string &child : closure.childObjectIdentifiers()) {
			hash.appendString(child);
		}
		append_bounds(&hash, closure.closedBounds());
	}
	for (const HelicalGlassDrop &glass : architecture.glassDrops()) {
		hash.appendString(glass.identifier());
		hash.appendString(glass.parentClosureIdentifier());
		hash.appendString(glass.glassSurface().identifier());
		for (const glm::vec3 &point : glass.glassSurface().perimeterPoints()) {
			hash.appendVector3(point);
		}
		append_bounds(&hash, glass.glassSurface().localBounds());
		hash.appendString(glass.barrelSurface().identifier());
		hash.appendVector3(glass.barrelSurface().axisOrigin());
		hash.appendVector3(glass.barrelSurface().axisDirection());
		hash.appendValue(glass.barrelSurface().radius());
		hash.appendValue(glass.barrelSurface().axialLength());
		hash.appendVector3(glass.helixAxis());
		hash.appendValue(glass.pitch());
		hash.appendValue(glass.rotationRateDegrees());
		hash.appendString(glass.frontChannel().identifier());
		append_curve(&hash, glass.frontChannel().centreLine());
		hash.appendValue(glass.frontChannel().channelWidth());
		hash.appendValue(glass.frontChannel().channelDepth());
		hash.appendValue(glass.frontChannel().clearance());
		hash.appendString(glass.rearChannel().identifier());
		append_curve(&hash, glass.rearChannel().centreLine());
		hash.appendValue(glass.rearChannel().channelWidth());
		hash.appendValue(glass.rearChannel().channelDepth());
		hash.appendValue(glass.rearChannel().clearance());
		hash.appendValue(glass.upperLimit());
		hash.appendValue(glass.lowerLimit());
		append_bounds(&hash, glass.doorCavity());
	}
	for (const VariableSealSweep &seal : architecture.sealSweeps()) {
		hash.appendString(seal.identifier());
		append_curve(&hash, seal.path());
		for (const VariableSealSectionStation &station : seal.sectionStations()) {
			hash.appendValue(station.parameter());
			hash.appendValue(station.width());
			hash.appendValue(station.height());
			hash.appendValue(station.compressionTarget());
		}
		hash.appendString(seal.material());
		for (const std::string &target : seal.contactTargetIdentifiers()) {
			hash.appendString(target);
		}
	}

	hash.appendString(architecture.bodyInWhite().identifier());
	for (const AutomotiveStructuralMember &member : architecture.bodyInWhite().members()) {
		hash.appendString(member.identifier());
		hash.appendString(member.structuralRole());
		append_curve(&hash, member.centreLine());
		for (const AutomotiveStructuralSectionStation &station : member.sectionStations()) {
			hash.appendValue(station.parameter());
			hash.appendValue(station.width());
			hash.appendValue(station.height());
			hash.appendValue(station.wallThickness());
			hash.appendValue(station.flangeWidth());
		}
		for (const AxisAlignedBounds &hole : member.holes()) append_bounds(&hash, hole);
		for (const std::string &reinforcement : member.reinforcementIdentifiers()) {
			hash.appendString(reinforcement);
		}
		hash.appendString(member.material());
		hash.appendString(member.provenance());
	}
	for (const BIWJoint &joint : architecture.bodyInWhite().joints()) {
		hash.appendString(joint.identifier());
		for (const std::string &member : joint.memberIdentifiers()) hash.appendString(member);
		for (const std::string &surface : joint.overlapSurfaceIdentifiers()) {
			hash.appendString(surface);
		}
		hash.appendValue(static_cast<int>(joint.joiningMethod()));
		hash.appendString(joint.localReinforcementIdentifier());
		hash.appendVector3(joint.jointPosition());
	}

	for (const SuspensionHardpointModel &model : architecture.suspensionModels()) {
		hash.appendString(model.identifier());
		hash.appendValue(static_cast<int>(model.corner()));
		hash.appendValue(model.minimumTravel());
		hash.appendValue(model.maximumTravel());
		hash.appendValue(model.minimumSteeringDegrees());
		hash.appendValue(model.maximumSteeringDegrees());
		for (const SuspensionHardpoint &hardpoint : model.hardpoints()) {
			hash.appendString(hardpoint.identifier());
			hash.appendValue(static_cast<int>(hardpoint.role()));
			hash.appendValue(static_cast<int>(hardpoint.corner()));
			hash.appendVector3(hardpoint.position());
			hash.appendValue(static_cast<int>(hardpoint.evidenceClassification()));
			hash.appendValue(hardpoint.tolerance());
		}
		for (const KinematicLink &link : model.links()) {
			hash.appendString(link.identifier());
			hash.appendString(link.firstHardpointIdentifier());
			hash.appendString(link.secondHardpointIdentifier());
			hash.appendValue(link.nominalLength());
		}
	}
	for (const WheelPoseFunction &function : architecture.wheelPoseFunctions()) {
		hash.appendString(function.identifier());
		hash.appendString(function.hardpointModelIdentifier());
		for (const SolvedWheelPose &pose : function.sampledPoses()) {
			hash.appendValue(pose.state().suspensionTravel());
			hash.appendValue(pose.state().steeringDegrees());
			hash.appendVector3(pose.centre());
			hash.appendVector3(pose.axleDirection());
			hash.appendValue(pose.camberDegrees());
			hash.appendValue(pose.toeDegrees());
		}
	}
	for (const WheelSweptEnvelope &envelope : architecture.wheelSweptEnvelopes()) {
		hash.appendString(envelope.identifier());
		for (const AxisAlignedBounds &sample : envelope.sampledTyreBounds()) {
			append_bounds(&hash, sample);
		}
		append_bounds(&hash, envelope.sweptBounds());
	}
	for (const TyreGeometry &tyre : architecture.tyreGeometries()) {
		hash.appendString(tyre.identifier());
		hash.appendValue(tyre.sectionWidth());
		hash.appendValue(tyre.aspectRatio());
		hash.appendValue(tyre.rimRadius());
		hash.appendValue(tyre.crownRadius());
		hash.appendValue(tyre.shoulderRadius());
		hash.appendValue(tyre.sidewallBulge());
		hash.appendValue(tyre.treadDepth());
		hash.appendValue(tyre.unloadedRadius());
		hash.appendValue(tyre.loadedRadius());
	}
	for (const AutomotiveRimGeometry &rim : architecture.rimGeometries()) {
		hash.appendString(rim.identifier());
		hash.appendValue(static_cast<int>(rim.corner()));
		hash.appendValue(rim.outerRadius());
		hash.appendValue(rim.barrelRadius());
		hash.appendValue(rim.width());
		hash.appendValue(rim.spokeCount());
		hash.appendString(rim.material());
	}
	for (const AutomotiveBrakeGeometry &brake : architecture.brakeGeometries()) {
		hash.appendString(brake.identifier());
		hash.appendValue(static_cast<int>(brake.corner()));
		hash.appendValue(brake.rotorRadius());
		hash.appendValue(brake.rotorThickness());
		append_bounds(&hash, brake.caliperBounds());
		hash.appendValue(brake.caliperPistonCount());
		hash.appendString(brake.rotorMaterial());
	}
	for (const WheelHouse &house : architecture.wheelHouses()) {
		hash.appendString(house.identifier());
		hash.appendValue(static_cast<int>(house.corner()));
		append_bounds(&hash, house.cavityBounds());
		hash.appendValue(house.minimumClearance());
	}
	for (const AeroGeometryComponent &component : architecture.aeroGeometry().components()) {
		hash.appendString(component.identifier());
		hash.appendValue(static_cast<int>(component.role()));
		append_bounds(&hash, component.bounds());
		hash.appendValue(component.validationWeight());
	}

	for (const VehicleFitTermWeight &weight : architecture.fitObjective().termWeights()) {
		hash.appendValue(static_cast<int>(weight.term()));
		hash.appendValue(weight.weight());
	}
	for (const VehicleFitConstraint &constraint : architecture.fitObjective().constraints()) {
		hash.appendString(constraint.identifier());
		hash.appendValue(static_cast<int>(constraint.term()));
		hash.appendValue(static_cast<int>(constraint.strength()));
		hash.appendValue(constraint.tolerance());
		hash.appendValue(constraint.measuredResidual());
	}
	hash.appendValue(architecture.fitObjective().patchCountPenalty());
	hash.appendValue(architecture.fitObjective().controlPointPenalty());
	hash.appendValue(architecture.fitObjective().spanPenalty());
	for (const VehicleFitResidualComponent &component :
	     architecture.residualReport().components()) {
		hash.appendValue(static_cast<int>(component.term()));
		hash.appendValue(component.rawError());
		hash.appendValue(component.weight());
		hash.appendValue(component.weightedError());
	}
	for (const std::string &failure : architecture.residualReport().hardFailureIdentifiers()) {
		hash.appendString(failure);
	}
	hash.appendValue(architecture.residualReport().totalWeightedError());
	for (const HighlightFlowResidual &residual : architecture.highlightFlowReport().residuals()) {
		hash.appendString(residual.connectionIdentifier());
		hash.appendValue(residual.positionError());
		hash.appendValue(residual.tangentError());
		hash.appendValue(residual.curvatureError());
		hash.appendValue(residual.normalFlowError());
	}
	hash.appendValue(architecture.highlightFlowReport().patchCount());
	hash.appendValue(architecture.highlightFlowReport().controlPointCount());
	hash.appendValue(architecture.highlightFlowReport().spanCount());
	return hash.value();
}

std::uint64_t VehicleMvp25DeterministicHashService::calculateResidualHash(
	const VehicleFitResidualReport &report) const
{
	VehicleMvp25HashAccumulator hash;
	hash.appendString("MVPv2.5-ResidualReport-v1");
	for (const VehicleFitResidualComponent &component : report.components()) {
		hash.appendValue(static_cast<int>(component.term()));
		hash.appendValue(component.rawError());
		hash.appendValue(component.weight());
		hash.appendValue(component.weightedError());
	}
	for (const std::string &failure : report.hardFailureIdentifiers()) {
		hash.appendString(failure);
	}
	hash.appendValue(report.totalWeightedError());
	return hash.value();
}
