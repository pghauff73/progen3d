#include "vehicle/service/VehicleMvp25ValidationService.h"

#include "vehicle/service/AutomotiveClosureGeometryService.h"
#include "vehicle/service/VehicleChassisKinematicsService.h"
#include "vehicle/service/VehicleClassAValidationService.h"
#include "vehicle/service/VehicleProjectionService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace {

bool finite(float value)
{
	return std::isfinite(value);
}

bool finite(const glm::vec2 &value)
{
	return finite(value.x) && finite(value.y);
}

bool finite(const glm::vec3 &value)
{
	return finite(value.x) && finite(value.y) && finite(value.z);
}

bool valid_bounds(const AxisAlignedBounds &bounds)
{
	return bounds.valid && finite(bounds.min) && finite(bounds.max) &&
	       bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y &&
	       bounds.min.z <= bounds.max.z;
}

bool bounds_contain(
	const AxisAlignedBounds &outer,
	const AxisAlignedBounds &inner,
	float tolerance = 1.0e-5f)
{
	return valid_bounds(outer) && valid_bounds(inner) &&
	       inner.min.x >= outer.min.x - tolerance &&
	       inner.min.y >= outer.min.y - tolerance &&
	       inner.min.z >= outer.min.z - tolerance &&
	       inner.max.x <= outer.max.x + tolerance &&
	       inner.max.y <= outer.max.y + tolerance &&
	       inner.max.z <= outer.max.z + tolerance;
}

void add_issue(
	VehicleValidationReport *report,
	VehicleDiagnosticCode code,
	const std::string &message,
	const std::string &identifier = {})
{
	if (identifier.empty()) report->addIssue(VehicleValidationIssue(code, message));
	else report->addIssue(VehicleValidationIssue(code, message, {identifier}));
}

bool valid_station_series(const std::vector<AutomotiveScalarStation> &stations)
{
	if (stations.empty()) return false;
	float previous = -1.0f;
	for (const AutomotiveScalarStation &station : stations) {
		if (!finite(station.parameter()) || !finite(station.value()) ||
		    station.parameter() < 0.0f || station.parameter() > 1.0f ||
		    station.parameter() <= previous || station.value() <= 0.0f) {
			return false;
		}
		previous = station.parameter();
	}
	return true;
}

bool valid_line_observation_set(
	const std::vector<CharacterLineObservation> &observations,
	const std::string &camera_identifier)
{
	if (observations.empty()) return false;
	std::unordered_set<std::string> identifiers;
	for (const CharacterLineObservation &observation : observations) {
		if (observation.identifier().empty() ||
		    !identifiers.insert(observation.identifier()).second ||
		    observation.cameraIdentifier() != camera_identifier ||
		    observation.imagePoints().size() < 2u ||
		    observation.uncertainty() <= 0.0f ||
		    observation.confidence() <= 0.0f) {
			return false;
		}
		for (const glm::vec2 &point : observation.imagePoints()) {
			if (!finite(point)) return false;
		}
	}
	return true;
}

bool contains_role(
	const std::vector<AeroGeometryComponent> &components,
	AeroGeometryRole role)
{
	return std::any_of(
		components.begin(), components.end(),
		[role](const AeroGeometryComponent &component) {
			return component.role() == role;
		});
}

} // namespace

VehicleValidationReport VehicleMvp25ValidationService::validate(
	const VehicleMvp25Architecture &architecture) const
{
	VehicleValidationReport report;
	const VehicleReferenceFrame &frame = architecture.referenceFrame();
	const glm::vec3 longitudinal = frame.longitudinalAxis();
	const glm::vec3 lateral = frame.lateralAxis();
	const glm::vec3 vertical = frame.verticalAxis();
	const bool orthonormal = finite(frame.origin()) && finite(longitudinal) &&
		finite(lateral) && finite(vertical) &&
		std::fabs(glm::length(longitudinal) - 1.0f) <= 1.0e-4f &&
		std::fabs(glm::length(lateral) - 1.0f) <= 1.0e-4f &&
		std::fabs(glm::length(vertical) - 1.0f) <= 1.0e-4f &&
		std::fabs(glm::dot(longitudinal, lateral)) <= 1.0e-4f &&
		std::fabs(glm::dot(longitudinal, vertical)) <= 1.0e-4f &&
		std::fabs(glm::dot(lateral, vertical)) <= 1.0e-4f &&
		glm::dot(glm::cross(lateral, vertical), longitudinal) > 0.999f;
	if (!orthonormal || !finite(frame.groundPlane()) ||
	    frame.fiducialDatums().empty()) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidVehicleReferenceFrame,
			"MVPv2.5 requires a right-handed orthonormal SAE reference frame and fiducial datums.",
			architecture.identifier());
	}

	const VehiclePackageEvidence &package = architecture.packageEvidence();
	const float measured_wheelbase = glm::length(
		package.frontAxleCentre() - package.rearAxleCentre());
	if (!finite(package.overallLength()) || package.overallLength() <= 0.0f ||
	    !finite(package.overallWidth()) || package.overallWidth() <= 0.0f ||
	    !finite(package.overallHeight()) || package.overallHeight() <= 0.0f ||
	    !finite(package.wheelbase()) || package.wheelbase() <= 0.0f ||
	    package.wheelbase() >= package.overallLength() ||
	    package.frontTrack() <= 0.0f || package.rearTrack() <= 0.0f ||
	    std::fabs(measured_wheelbase - package.wheelbase()) > package.tolerance()) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidWheelbase,
			"Hard package dimensions and axle centres must be positive and wheelbase-consistent.",
			architecture.identifier());
	}
	if (architecture.occupantPackages().size() < 2u) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidOccupantPackage,
			"The acceptance vehicle requires front and rear occupant packages.");
	}
	for (const OccupantPackage &occupant : architecture.occupantPackages()) {
		if (occupant.identifier().empty() || !finite(occupant.hPoint()) ||
		    !finite(occupant.eyePoint()) || !finite(occupant.heelPoint()) ||
		    glm::length(occupant.torsoDirection()) < 1.0e-6f ||
		    !valid_bounds(occupant.kneeEnvelope()) ||
		    !valid_bounds(occupant.headEnvelope()) ||
		    !valid_bounds(occupant.reachEnvelope())) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidOccupantPackage,
				"Occupant package requires finite H/eye/heel points, torso direction, and valid envelopes.",
				occupant.identifier());
		}
	}

	std::set<VehicleReferenceView> reference_views;
	std::unordered_set<std::string> camera_identifiers;
	for (const VehicleViewObservation &view : architecture.observations().observations()) {
		reference_views.insert(view.camera().view());
		camera_identifiers.insert(view.camera().identifier());
		const VehicleCameraModel &camera = view.camera();
		const float determinant = glm::determinant(camera.worldToCamera());
		if (camera.identifier().empty() || view.imageIdentifier().empty() ||
		    view.silhouette().size() < 3u || !finite(determinant) ||
		    std::fabs(determinant) < 1.0e-8f ||
		    !finite(camera.principalPoint()) || !finite(camera.imageDimensions()) ||
		    camera.imageDimensions().x <= 0.0f || camera.imageDimensions().y <= 0.0f ||
		    view.landmarkIdentifiers().empty() ||
		    !valid_line_observation_set(view.characterLines(), camera.identifier()) ||
		    !valid_line_observation_set(view.panelLines(), camera.identifier()) ||
		    !valid_line_observation_set(view.glazingLines(), camera.identifier()) ||
		    !valid_line_observation_set(view.wheelEllipses(), camera.identifier())) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidViewEvidence,
				"Vehicle observations require finite invertible cameras, silhouettes, landmarks, character lines, panel lines, glazing lines, and wheel ellipses.",
				view.identifier());
		}
	}
	const VehicleReferenceView required_views[] = {
		VehicleReferenceView::Front, VehicleReferenceView::Rear,
		VehicleReferenceView::Left, VehicleReferenceView::Right,
		VehicleReferenceView::Top};
	for (VehicleReferenceView view : required_views) {
		if (reference_views.count(view) == 0u) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidViewEvidence,
				"MVPv2.5 requires front, rear, left, right, and top observations.");
		}
	}

	const AutomotiveWireframe &wireframe = architecture.wireframe();
	std::unordered_set<std::string> landmark_identifiers;
	for (const VehicleLandmark &landmark : wireframe.landmarks()) {
		if (landmark.identifier().empty() ||
		    !landmark_identifiers.insert(landmark.identifier()).second ||
		    !finite(landmark.position()) || landmark.observations().empty()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidAutomotiveWireframe,
				"Vehicle landmarks require unique identity, finite position, and observations.",
				landmark.identifier());
		}
		for (const VehicleLandmarkObservation &observation : landmark.observations()) {
			if (camera_identifiers.count(observation.cameraIdentifier()) == 0u ||
			    !finite(observation.imagePoint()) || observation.confidence() <= 0.0f ||
			    observation.uncertainty() <= 0.0f) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidViewEvidence,
					"Landmark observations require an existing camera and positive uncertainty/confidence.",
					landmark.identifier());
			}
		}
	}
	if (wireframe.landmarks().size() < 25u || wireframe.edges().empty() ||
	    wireframe.shapePrior().strength() != VehicleConstraintStrength::Soft) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidAutomotiveWireframe,
			"Acceptance wireframe requires at least 25 landmarks, topology edges, and a soft shape prior.",
			wireframe.identifier());
	}
	for (const AutomotiveWireframeEdge &edge : wireframe.edges()) {
		if (landmark_identifiers.count(edge.firstLandmarkIdentifier()) == 0u ||
		    landmark_identifiers.count(edge.secondLandmarkIdentifier()) == 0u) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidAutomotiveWireframe,
				"Wireframe edges must reference existing landmarks.", edge.identifier());
		}
	}

	report.append(VehicleClassAValidationService().validateGraph(
		architecture.classASurfaces()));
	if (architecture.classASurfaces().patches().size() < 15u) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidClassASurfaceGraph,
			"Acceptance Class-A graph requires at least fifteen patches.");
	}
	for (const HighlightFlowResidual &residual :
	     architecture.highlightFlowReport().residuals()) {
		if (!finite(residual.positionError()) || !finite(residual.tangentError()) ||
		    !finite(residual.curvatureError()) || !finite(residual.normalFlowError())) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidClassASurfaceGraph,
				"Highlight-flow residuals must be finite.",
				residual.connectionIdentifier());
		}
	}

	for (const AutomotivePanelGap &gap : architecture.panelGaps()) {
		std::string curve_diagnostic;
		if (gap.identifier().empty() || !gap.seamCurve().isValid(&curve_diagnostic) ||
		    !valid_station_series(gap.gapWidthStations()) ||
		    !valid_station_series(gap.primaryRadiusStations()) ||
		    !valid_station_series(gap.secondaryRadiusStations()) ||
		    gap.primaryFlange().length() <= 0.0f ||
		    gap.secondaryFlange().length() <= 0.0f) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidAutomotivePanelGap,
				"Automotive panel gap requires valid seam, positive variable widths/radii, and two flanges.",
				gap.identifier());
		}
	}
	if (architecture.panelGaps().size() < 6u || architecture.rolledEdges().size() < 6u) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidAutomotivePanelGap,
			"Acceptance model requires six rolled closure gaps and rolled edges.");
	}
	std::unordered_set<std::string> aperture_identifiers;
	for (const BodySideAperture &aperture : architecture.bodySideApertures()) {
		std::string curve_diagnostic;
		aperture_identifiers.insert(aperture.identifier());
		if (aperture.identifier().empty() || !aperture.bLine().isValid(&curve_diagnostic) ||
		    aperture.hingePillarIdentifier().empty() || aperture.roofRailIdentifier().empty() ||
		    aperture.rockerIdentifier().empty() || aperture.jSurfaceIdentifier().empty() ||
		    aperture.glassSurfaceIdentifier().empty() ||
		    aperture.sealSeatIdentifier().empty()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidBodySideAperture,
				"Body-side aperture requires pillars, roof rail, rocker, B-line, J-surface, glass, flange, and seal seat.",
				aperture.identifier());
		}
	}
	if (architecture.bodySideApertures().size() != 4u ||
	    architecture.doorEgressSurfaces().size() != 4u) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidBodySideAperture,
			"Five-door hatch acceptance model requires four body-side apertures and egress surfaces.");
	}

	for (const AutomotiveClosure &closure : architecture.closures()) {
		if (closure.identifier().empty() || !valid_bounds(closure.closedBounds()) ||
		    glm::length(closure.hingeStudy().axis()) < 0.999f ||
		    !valid_bounds(closure.hingeStudy().sourceBounds()) ||
		    !valid_bounds(closure.hingeStudy().sweptBounds()) ||
		    ((closure.type() == AutomotiveClosureType::FrontDoor ||
		      closure.type() == AutomotiveClosureType::RearDoor) &&
		     aperture_identifiers.count(closure.apertureIdentifier()) == 0u)) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidClosureAssembly,
				"Closure requires valid hinge study, bounds, and side-door aperture reference.",
				closure.identifier());
		}
	}
	if (architecture.closures().size() < 6u) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidClosureAssembly,
			"Acceptance model requires four doors, bonnet, and rear hatch.");
	}
	for (const HelicalGlassDrop &glass : architecture.glassDrops()) {
		if (glass.identifier().empty() || glass.pitch() <= 0.0f ||
		    std::fabs(glass.rotationRateDegrees()) <= 1.0e-6f ||
		    !valid_bounds(glass.glassSurface().localBounds()) ||
		    !valid_bounds(glass.doorCavity()) ||
		    !AutomotiveClosureGeometryService().glassRemainsInsideDoorCavity(
			    glass, 9u, 1.0e-4f)) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidHelicalGlassDrop,
				"Helical glass drop requires translation, rotation, channels, and full cavity containment.",
				glass.identifier());
		}
	}
	if (architecture.glassDrops().size() != 4u) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidHelicalGlassDrop,
			"Acceptance model requires four helical side-glass drops.");
	}
	for (const VariableSealSweep &seal : architecture.sealSweeps()) {
		std::string curve_diagnostic;
		float previous_parameter = -1.0f;
		bool valid_sections = seal.sectionStations().size() >= 2u;
		for (const VariableSealSectionStation &station : seal.sectionStations()) {
			valid_sections = valid_sections && finite(station.parameter()) &&
				station.parameter() > previous_parameter && station.width() > 0.0f &&
				station.height() > 0.0f && station.compressionTarget() >= 0.0f &&
				station.compressionTarget() <= 1.0f;
			previous_parameter = station.parameter();
		}
		if (!seal.path().isValid(&curve_diagnostic) || !valid_sections ||
		    seal.contactTargetIdentifiers().empty()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidClosureAssembly,
				"Variable seal sweep requires a valid path, ordered positive sections, and contact targets.",
				seal.identifier());
		}
	}

	const BodyInWhite &body_in_white = architecture.bodyInWhite();
	std::unordered_set<std::string> member_identifiers;
	for (const AutomotiveStructuralMember &member : body_in_white.members()) {
		std::string curve_diagnostic;
		member_identifiers.insert(member.identifier());
		bool valid_sections = member.sectionStations().size() >= 2u;
		float previous_parameter = -1.0f;
		for (const AutomotiveStructuralSectionStation &station : member.sectionStations()) {
			valid_sections = valid_sections && station.parameter() > previous_parameter &&
				station.width() > 0.0f && station.height() > 0.0f &&
				station.wallThickness() > 0.0f;
			previous_parameter = station.parameter();
		}
		bool valid_holes = !member.holes().empty();
		for (const AxisAlignedBounds &hole : member.holes()) {
			valid_holes = valid_holes && valid_bounds(hole) &&
				hole.half_extents.x > 0.0f && hole.half_extents.y > 0.0f &&
				hole.half_extents.z > 0.0f;
		}
		if (member.identifier().empty() || !member.centreLine().isValid(&curve_diagnostic) ||
		    !valid_sections || !valid_holes ||
		    member.reinforcementIdentifiers().empty() ||
		    member.material().empty() || member.provenance().empty()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidBodyInWhite,
				"BIW members require variable positive sections, holes, reinforcements, material, provenance, and a valid path.",
				member.identifier());
		}
	}
	std::unordered_map<std::string, std::vector<std::string>> member_adjacency;
	for (const BIWJoint &joint : body_in_white.joints()) {
		if (joint.memberIdentifiers().size() < 2u) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidBodyInWhite,
				"BIW joint must connect at least two members.", joint.identifier());
			continue;
		}
		for (const std::string &member : joint.memberIdentifiers()) {
			if (member_identifiers.count(member) == 0u) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidBodyInWhite,
					"BIW joint references an unknown structural member.", joint.identifier());
			}
		}
		for (std::size_t first = 0u; first < joint.memberIdentifiers().size(); ++first) {
			for (std::size_t second = first + 1u;
			     second < joint.memberIdentifiers().size(); ++second) {
				member_adjacency[joint.memberIdentifiers()[first]].push_back(
					joint.memberIdentifiers()[second]);
				member_adjacency[joint.memberIdentifiers()[second]].push_back(
					joint.memberIdentifiers()[first]);
			}
		}
	}
	if (!body_in_white.members().empty()) {
		std::unordered_set<std::string> visited;
		std::queue<std::string> pending;
		pending.push(body_in_white.members().front().identifier());
		while (!pending.empty()) {
			const std::string current = pending.front();
			pending.pop();
			if (!visited.insert(current).second) continue;
			for (const std::string &neighbor : member_adjacency[current]) pending.push(neighbor);
		}
		if (visited.size() != body_in_white.members().size()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidBodyInWhite,
				"Body-in-white member/joint graph must be connected.",
				body_in_white.identifier());
		}
	}

	if (architecture.suspensionModels().size() != 4u ||
	    architecture.wheelPoseFunctions().size() != 4u ||
	    architecture.wheelSweptEnvelopes().size() != 4u ||
	    architecture.tyreGeometries().size() != 4u ||
	    architecture.wheelHouses().size() != 4u) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidSuspensionHardpoint,
			"Acceptance chassis requires four hardpoint, wheel-pose, swept-envelope, tyre, and wheel-house models.");
	}
	if (architecture.rimGeometries().size() != 4u) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidRimGeometry,
			"Acceptance wheel hierarchy requires four rim geometry records.");
	}
	if (architecture.brakeGeometries().size() != 4u) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidBrakeGeometry,
			"Acceptance wheel hierarchy requires four brake geometry records.");
	}
	for (const SuspensionHardpointModel &model : architecture.suspensionModels()) {
		VehicleValidationReport model_report =
			VehicleChassisKinematicsService().validateHardpointModel(model);
		if (!model_report.isValid()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidSuspensionHardpoint,
				"Suspension hardpoint model failed link/reference validation.",
				model.identifier());
		}
	}
	for (const TyreGeometry &tyre : architecture.tyreGeometries()) {
		if (tyre.identifier().empty() || tyre.sectionWidth() <= 0.0f ||
		    tyre.aspectRatio() <= 0.0f || tyre.rimRadius() <= 0.0f ||
		    tyre.crownRadius() <= 0.0f || tyre.shoulderRadius() <= 0.0f ||
		    tyre.sidewallBulge() < 0.0f || tyre.treadDepth() <= 0.0f ||
		    tyre.loadedRadius() <= 0.0f || tyre.unloadedRadius() <= tyre.loadedRadius()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidTyreGeometry,
				"Tyre geometry requires section, aspect ratio, crown, shoulder, bulge, tread, and distinct loaded/unloaded radii.",
				tyre.identifier());
		}
	}
	for (const AutomotiveRimGeometry &rim : architecture.rimGeometries()) {
		if (rim.identifier().empty() || !finite(rim.outerRadius()) ||
		    !finite(rim.barrelRadius()) || !finite(rim.width()) ||
		    rim.outerRadius() <= 0.0f || rim.barrelRadius() <= 0.0f ||
		    rim.barrelRadius() >= rim.outerRadius() || rim.width() <= 0.0f ||
		    rim.spokeCount() < 3u || rim.material().empty()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidRimGeometry,
				"Rim geometry requires ordered outer/barrel radii, positive width, at least three spokes, and material evidence.",
				rim.identifier());
		}
	}
	for (const AutomotiveBrakeGeometry &brake : architecture.brakeGeometries()) {
		if (brake.identifier().empty() || !finite(brake.rotorRadius()) ||
		    !finite(brake.rotorThickness()) || brake.rotorRadius() <= 0.0f ||
		    brake.rotorThickness() <= 0.0f ||
		    !valid_bounds(brake.caliperBounds()) ||
		    brake.caliperPistonCount() == 0u || brake.rotorMaterial().empty()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidBrakeGeometry,
				"Brake geometry requires positive rotor dimensions, valid caliper bounds, piston evidence, and rotor material.",
				brake.identifier());
		}
	}
	const std::size_t wheel_hierarchy_count = std::min({
		architecture.suspensionModels().size(),
		architecture.tyreGeometries().size(),
		architecture.rimGeometries().size(),
		architecture.brakeGeometries().size(),
		architecture.wheelHouses().size()});
	for (std::size_t index = 0u; index < wheel_hierarchy_count; ++index) {
		const VehicleCornerLocation expected_corner =
			architecture.suspensionModels()[index].corner();
		const TyreGeometry &tyre = architecture.tyreGeometries()[index];
		const AutomotiveRimGeometry &rim = architecture.rimGeometries()[index];
		const AutomotiveBrakeGeometry &brake = architecture.brakeGeometries()[index];
		const WheelHouse &wheel_house = architecture.wheelHouses()[index];
		if (rim.corner() != expected_corner ||
		    rim.outerRadius() > tyre.rimRadius() + 1.0e-5f) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidRimGeometry,
				"Rim corner association and tyre bead-seat radius must match the suspension wheel hierarchy.",
				rim.identifier());
		}
		if (brake.corner() != expected_corner ||
		    brake.rotorRadius() >= rim.barrelRadius() ||
		    !bounds_contain(wheel_house.cavityBounds(), brake.caliperBounds())) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidBrakeGeometry,
				"Brake rotor and caliper must match the wheel corner and remain inside the rim/wheel-house package.",
				brake.identifier());
		}
		if (wheel_house.corner() != expected_corner) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidWheelSweptEnvelope,
				"Wheel-house corner association must match the suspension wheel hierarchy.",
				wheel_house.identifier());
		}
	}
	const std::size_t wheel_count = std::min(
		architecture.wheelHouses().size(), architecture.wheelSweptEnvelopes().size());
	for (std::size_t index = 0u; index < wheel_count; ++index) {
		if (!VehicleChassisKinematicsService().wheelHouseContainsEnvelope(
			    architecture.wheelHouses()[index],
			    architecture.wheelSweptEnvelopes()[index], 1.0e-4f)) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidWheelSweptEnvelope,
				"Wheel house must contain the complete sampled tyre envelope plus clearance.",
				architecture.wheelHouses()[index].identifier());
		}
	}

	const std::vector<AeroGeometryComponent> &aero = architecture.aeroGeometry().components();
	const AeroGeometryRole required_aero_roles[] = {
		AeroGeometryRole::UpperBody, AeroGeometryRole::Wheel,
		AeroGeometryRole::WheelHouse, AeroGeometryRole::Underbody,
		AeroGeometryRole::Splitter, AeroGeometryRole::CoolingOpening,
		AeroGeometryRole::Diffuser, AeroGeometryRole::Spoiler};
	for (AeroGeometryRole role : required_aero_roles) {
		if (!contains_role(aero, role)) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidAeroGeometry,
				"Aero validation requires upper body, wheels, wheel houses, underbody, splitter, cooling, diffuser, and spoiler geometry.");
		}
	}

	std::set<VehicleFitResidualTerm> weighted_terms;
	for (const VehicleFitTermWeight &weight : architecture.fitObjective().termWeights()) {
		weighted_terms.insert(weight.term());
		if (!finite(weight.weight()) || weight.weight() < 0.0f) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidVehicleFitObjective,
				"Fit objective weights must be finite and non-negative.");
		}
	}
	if (weighted_terms.size() != 9u || !architecture.residualReport().isValid() ||
	    architecture.residualReport().components().size() != 9u ||
	    !finite(architecture.residualReport().totalWeightedError())) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidVehicleFitObjective,
			"Fit objective must expose all nine residual terms and no failed hard constraints.");
	}
	return report;
}
