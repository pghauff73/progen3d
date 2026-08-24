#include "vehicle/service/VehicleFittingValidationService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
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

bool normalized_value(float value)
{
	return finite(value) && value >= 0.0f && value <= 1.0f;
}

bool valid_bounds(const AxisAlignedBounds &bounds)
{
	return bounds.valid && finite(bounds.min) && finite(bounds.max) &&
	       bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y &&
	       bounds.min.z <= bounds.max.z;
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

template <typename Object, typename IdentifierAccessor>
std::unordered_set<std::string> collect_identifiers(
	const std::vector<Object> &objects,
	IdentifierAccessor identifier_accessor,
	VehicleDiagnosticCode duplicate_code,
	const std::string &object_kind,
	VehicleValidationReport *report)
{
	std::unordered_set<std::string> identifiers;
	for (const Object &object : objects) {
		const std::string &identifier = identifier_accessor(object);
		if (identifier.empty()) {
			add_issue(report, duplicate_code, object_kind + " requires an identifier.");
		}
		else if (!identifiers.insert(identifier).second) {
			add_issue(
				report, duplicate_code,
				object_kind + " identifier '" + identifier + "' is duplicated.",
				identifier);
		}
	}
	return identifiers;
}

bool validate_relationship_cycle(
	const std::string &identifier,
	const std::unordered_map<std::string, std::vector<std::string>> &dependencies,
	std::unordered_set<std::string> *visiting,
	std::unordered_set<std::string> *visited)
{
	if (visiting->count(identifier) != 0u) return false;
	if (visited->count(identifier) != 0u) return true;
	visiting->insert(identifier);
	const auto dependency = dependencies.find(identifier);
	if (dependency != dependencies.end()) {
		for (const std::string &child : dependency->second) {
			if (dependencies.count(child) != 0u &&
			    !validate_relationship_cycle(child, dependencies, visiting, visited)) {
				return false;
			}
		}
	}
	visiting->erase(identifier);
	visited->insert(identifier);
	return true;
}

} // namespace

VehicleValidationReport VehicleFittingValidationService::validate(
	const VehicleFittingArchitecture &architecture) const
{
	VehicleValidationReport report;
	if (architecture.identifier().empty()) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidViewEvidence,
			"Vehicle fitting architecture requires an identifier.");
	}

	std::set<VehicleReferenceView> available_views;
	std::unordered_set<std::string> view_identifiers;
	float total_view_weight = 0.0f;
	for (const VehicleViewEnvelope &view :
	     architecture.multiViewEnvelope().viewEnvelopes()) {
		if (view.identifier().empty() || !view_identifiers.insert(view.identifier()).second ||
		    view.cameraIdentifier().empty() || view.silhouetteSamples().size() < 3u ||
		    !finite(view.fittingWeight()) || view.fittingWeight() <= 0.0f) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidViewEvidence,
				"Each reference view requires a unique identifier, camera, at least three samples, and a positive fitting weight.",
				view.identifier());
		}
		available_views.insert(view.view());
		total_view_weight += view.fittingWeight();
		std::unordered_set<std::string> sample_names;
		for (const ViewSilhouetteSample &sample : view.silhouetteSamples()) {
			if (sample.semanticName().empty() ||
			    !sample_names.insert(sample.semanticName()).second ||
			    !finite(sample.imagePoint()) || sample.imagePoint().x < 0.0f ||
			    sample.imagePoint().x > 1.0f || sample.imagePoint().y < 0.0f ||
			    sample.imagePoint().y > 1.0f) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidViewEvidence,
					"View silhouette samples require unique names and normalized finite image coordinates.",
					view.identifier());
			}
		}
	}
	const VehicleReferenceView required_views[] = {
		VehicleReferenceView::Front, VehicleReferenceView::Rear,
		VehicleReferenceView::Left, VehicleReferenceView::Right,
		VehicleReferenceView::Top};
	for (VehicleReferenceView required_view : required_views) {
		if (available_views.count(required_view) == 0u) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidViewEvidence,
				"MultiViewEnvelope requires front, rear, left, right, and top evidence.");
		}
	}
	if (!finite(total_view_weight) || total_view_weight <= 0.0f ||
	    !finite(architecture.multiViewEnvelope().projectionTolerance()) ||
	    architecture.multiViewEnvelope().projectionTolerance() <= 0.0f) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidViewEvidence,
			"Multi-view fitting weights and projection tolerance must be positive and finite.");
	}

	const auto section_identifiers = collect_identifiers(
		architecture.crossSections(),
		[](const VehicleCrossSection &section) -> const std::string & {
			return section.identifier();
		},
		VehicleDiagnosticCode::InvalidBodySection, "VehicleCrossSection", &report);
	(void)section_identifiers;
	float previous_station = -INFINITY;
	const VehicleSectionLandmarkRole required_section_roles[] = {
		VehicleSectionLandmarkRole::CentreRoof,
		VehicleSectionLandmarkRole::RoofRail,
		VehicleSectionLandmarkRole::GlassShoulder,
		VehicleSectionLandmarkRole::Belt,
		VehicleSectionLandmarkRole::UpperShoulder,
		VehicleSectionLandmarkRole::LowerDoor,
		VehicleSectionLandmarkRole::Rocker,
		VehicleSectionLandmarkRole::Underbody,
		VehicleSectionLandmarkRole::CentreFloor};
	for (const VehicleCrossSection &section : architecture.crossSections()) {
		if (!finite(section.stationZ()) || section.stationZ() <= previous_station) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidBodySection,
				"VehicleCrossSection stations must be finite and strictly increasing.",
				section.identifier());
		}
		previous_station = section.stationZ();
		std::set<VehicleSectionLandmarkRole> roles;
		for (const VehicleSectionLandmark &landmark : section.landmarks()) {
			if (!finite(landmark.point()) ||
			    std::fabs(landmark.point().z - section.stationZ()) > 1.0e-4f ||
			    !roles.insert(landmark.role()).second) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidBodySection,
					"Section landmarks must be finite, unique by role, and lie on the section station.",
					section.identifier());
			}
		}
		for (VehicleSectionLandmarkRole required_role : required_section_roles) {
			if (roles.count(required_role) == 0u) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidBodySection,
					"VehicleCrossSection is missing a required semantic landmark.",
					section.identifier());
			}
		}
	}
	if (architecture.crossSections().size() < 3u) {
		add_issue(
			&report, VehicleDiagnosticCode::InvalidBodySection,
			"MVPv2 body fitting requires at least three semantic cross-sections.");
	}

	const auto landmark_identifiers = collect_identifiers(
		architecture.surfaceLandmarks(),
		[](const SurfaceLandmark &landmark) -> const std::string & {
			return landmark.identifier();
		},
		VehicleDiagnosticCode::InvalidSurfaceLandmark, "SurfaceLandmark", &report);
	for (const SurfaceLandmark &landmark : architecture.surfaceLandmarks()) {
		if (!finite(landmark.fittedPoint()) || !normalized_value(landmark.confidence())) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidSurfaceLandmark,
				"Surface landmarks require finite 3D points and confidence in [0,1].",
				landmark.identifier());
		}
		if (landmark.evidenceClassification() !=
				VehicleEvidenceClassification::EngineeringInference &&
		    landmark.observations().empty()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidSurfaceLandmark,
				"Observed or triangulated landmarks require image observations.",
				landmark.identifier());
		}
		for (const SurfaceLandmarkObservation &observation : landmark.observations()) {
			if (!finite(observation.imagePoint()) || observation.imagePoint().x < 0.0f ||
			    observation.imagePoint().x > 1.0f || observation.imagePoint().y < 0.0f ||
			    observation.imagePoint().y > 1.0f || !finite(observation.weight()) ||
			    observation.weight() <= 0.0f) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidSurfaceLandmark,
					"Landmark observations require normalized coordinates and positive weights.",
					landmark.identifier());
			}
		}
	}

	const auto curve_identifiers = collect_identifiers(
		architecture.characterCurveNetwork().curves(),
		[](const VehicleCharacterCurve &curve) -> const std::string & {
			return curve.identifier();
		},
		VehicleDiagnosticCode::InvalidCharacterCurveNetwork,
		"VehicleCharacterCurve", &report);
	for (const VehicleCharacterCurve &curve :
	     architecture.characterCurveNetwork().curves()) {
		std::string curve_diagnostic;
		if (!curve.curve().isValid(&curve_diagnostic)) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidCharacterCurveNetwork,
				curve_diagnostic, curve.identifier());
		}
		for (const std::string &landmark_identifier : curve.landmarkIdentifiers()) {
			if (landmark_identifiers.count(landmark_identifier) == 0u) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidCharacterCurveNetwork,
					"Character curve references missing landmark '" +
						landmark_identifier + "'.",
					curve.identifier());
			}
		}
	}
	for (const CharacterCurveRelationship &relationship :
	     architecture.characterCurveNetwork().relationships()) {
		if (curve_identifiers.count(relationship.sourceCurveIdentifier()) == 0u ||
		    curve_identifiers.count(relationship.targetCurveIdentifier()) == 0u ||
		    !finite(relationship.tolerance()) || relationship.tolerance() < 0.0f) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidCharacterCurveNetwork,
				"Character-curve relationships require existing curves and a non-negative tolerance.");
		}
	}

	std::unordered_set<std::string> patch_identifiers;
	for (const ProjectionConstrainedPatch &patch :
	     architecture.surfacePatchGraph().patches()) {
		const std::string &identifier = patch.initialPatch().identifier();
		if (identifier.empty() || !patch_identifiers.insert(identifier).second) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidSurfacePatchGraph,
				"Boundary patches require unique identifiers.", identifier);
		}
		for (const std::string &curve_identifier :
		     patch.initialPatch().boundaryCurveIdentifiers()) {
			if (curve_identifier.empty() || curve_identifiers.count(curve_identifier) == 0u) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidSurfacePatchGraph,
					"Boundary patch references an undefined character curve.", identifier);
			}
		}
		if (patch.projectionConstraints().empty() ||
		    !finite(patch.continuityWeight()) || patch.continuityWeight() < 0.0f ||
		    !finite(patch.complexityWeight()) || patch.complexityWeight() < 0.0f) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidSurfacePatchGraph,
				"Projection-constrained patches require projection evidence and finite non-negative weights.",
				identifier);
		}
		for (const std::string &landmark_identifier :
		     patch.freeLandmarkIdentifiers()) {
			if (landmark_identifiers.count(landmark_identifier) == 0u) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidSurfacePatchGraph,
					"Projection-constrained patch references an undefined free landmark.",
					identifier);
			}
		}
	}
	for (const SurfaceEdgeRelationship &relationship :
	     architecture.surfacePatchGraph().edgeRelationships()) {
		if (patch_identifiers.count(relationship.sourcePatchIdentifier()) == 0u ||
		    patch_identifiers.count(relationship.targetPatchIdentifier()) == 0u ||
		    relationship.sourceEdgeIdentifier().empty() ||
		    relationship.targetEdgeIdentifier().empty() ||
		    !finite(relationship.tolerance()) || relationship.tolerance() < 0.0f) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidSurfacePatchGraph,
				"Surface-edge relationships require existing patches, named edges, and non-negative tolerance.");
		}
	}

	const auto seam_identifiers = collect_identifiers(
		architecture.panelSeamLoops(),
		[](const PanelSeamLoop &seam) -> const std::string & { return seam.identifier(); },
		VehicleDiagnosticCode::InvalidAperture, "PanelSeamLoop", &report);
	for (const PanelSeamLoop &seam : architecture.panelSeamLoops()) {
		const auto &points = seam.closedBoundary().controlPoints();
		if (patch_identifiers.count(seam.hostPatchIdentifier()) == 0u ||
		    !seam.closedBoundary().isValid() || points.size() < 4u ||
		    glm::distance(points.front(), points.back()) > 1.0e-4f ||
		    !finite(seam.gapWidth()) || seam.gapWidth() <= 0.0f ||
		    !finite(seam.gapDepth()) || seam.gapDepth() < 0.0f) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidAperture,
				"Panel seam loops require an existing host patch, closed curve, positive gap width, and non-negative depth.",
				seam.identifier());
		}
	}

	const auto panel_identifiers = collect_identifiers(
		architecture.extractedPanels(),
		[](const ExtractedSurfacePanel &panel) -> const std::string & {
			return panel.identifier();
		},
		VehicleDiagnosticCode::InvalidAperture, "ExtractedSurfacePanel", &report);
	for (const ExtractedSurfacePanel &panel : architecture.extractedPanels()) {
		if (patch_identifiers.count(panel.sourcePatchIdentifier()) == 0u ||
		    seam_identifiers.count(panel.seamLoopIdentifier()) == 0u) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidAperture,
				"Extracted surface panels require existing source patches and seam loops.",
				panel.identifier());
		}
	}

	const auto aperture_identifiers = collect_identifiers(
		architecture.apertures(),
		[](const Aperture &aperture) -> const std::string & { return aperture.identifier(); },
		VehicleDiagnosticCode::InvalidAperture, "Aperture", &report);
	for (const Aperture &aperture : architecture.apertures()) {
		if (aperture.hostStructureIdentifier().empty() ||
		    seam_identifiers.count(aperture.outerBoundaryIdentifier()) == 0u ||
		    aperture.innerBoundaryIdentifier().empty() ||
		    !finite(aperture.flangeDepth()) || aperture.flangeDepth() <= 0.0f ||
		    aperture.sealSeatIdentifier().empty()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidAperture,
				"Apertures require a host, panel seam outer boundary, inner boundary, positive flange depth, and seal seat.",
				aperture.identifier());
		}
	}

	const auto closure_identifiers = collect_identifiers(
		architecture.closureAssemblies(),
		[](const ClosureAssembly &closure) -> const std::string & {
			return closure.identifier();
		},
		VehicleDiagnosticCode::InvalidClosureAssembly, "ClosureAssembly", &report);
	std::unordered_map<std::string, std::vector<std::string>> dependencies;
	for (const ClosureAssembly &closure : architecture.closureAssemblies()) {
		dependencies[closure.identifier()] = closure.childIdentifiers();
		if (closure.hostObjectIdentifier().empty() ||
		    aperture_identifiers.count(closure.apertureIdentifier()) == 0u ||
		    panel_identifiers.count(closure.outerPanelIdentifier()) == 0u ||
		    closure.innerPanelIdentifier().empty() || !normalized_value(closure.state()) ||
		    closure.sealIdentifiers().empty() || closure.latchIdentifiers().empty()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidClosureAssembly,
				"Closures require a host, aperture, outer and inner panels, seals, latches, and normalized state.",
				closure.identifier());
		}
		if (std::holds_alternative<HingePair>(closure.kinematicRelationship())) {
			const HingePair &hinge = std::get<HingePair>(closure.kinematicRelationship());
			if (hinge.identifier().empty() || glm::length(hinge.axis()) <= 1.0e-6f ||
			    !finite(hinge.maximumAngleDegrees()) ||
			    hinge.maximumAngleDegrees() <= 0.0f) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidClosureAssembly,
					"HingePair requires two distinct finite points and a positive angle.",
					closure.identifier());
			}
		}
		else {
			const FourBarJoint &joint = std::get<FourBarJoint>(closure.kinematicRelationship());
			if (joint.identifier().empty() ||
			    glm::distance(joint.bodyMountA(), joint.bodyMountB()) <= 1.0e-6f ||
			    glm::distance(joint.closureMountA(), joint.closureMountB()) <= 1.0e-6f ||
			    !finite(joint.maximumAngleDegrees()) || joint.maximumAngleDegrees() <= 0.0f) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidClosureAssembly,
					"FourBarJoint requires distinct body and closure mount pairs and a positive angle.",
					closure.identifier());
			}
			if (joint.evidenceClassification() == VehicleEvidenceClassification::Observed) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidEvidenceClassification,
					"Hidden four-bar mechanisms must be marked as engineering inference or triangulated evidence.",
					joint.identifier());
			}
		}
		for (const TelescopingLink &link : closure.supportLinks()) {
			if (link.identifier().empty() || !finite(link.bodyMount()) ||
			    !finite(link.closureMount()) || !finite(link.minimumLength()) ||
			    !finite(link.maximumLength()) || link.minimumLength() <= 0.0f ||
			    link.maximumLength() < link.minimumLength()) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidClosureAssembly,
					"Telescoping links require finite mounts and an ordered positive length interval.",
					link.identifier());
			}
			if (link.evidenceClassification() == VehicleEvidenceClassification::Observed) {
				add_issue(
					&report, VehicleDiagnosticCode::InvalidEvidenceClassification,
					"Hidden telescoping supports must not be classified as directly observed.",
					link.identifier());
			}
		}
		const SweptVolume &swept = closure.sweptVolume();
		if (!valid_bounds(swept.sourceBounds()) || !valid_bounds(swept.sweptBounds()) ||
		    swept.sampleCount() < 2u || !normalized_value(swept.minimumState()) ||
		    !normalized_value(swept.maximumState()) ||
		    swept.minimumState() > swept.maximumState()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidSweptVolume,
				"Closure swept volume requires valid source and swept bounds and at least two ordered samples.",
				closure.identifier());
		}
	}

	std::unordered_set<std::string> visiting;
	std::unordered_set<std::string> visited;
	for (const auto &dependency : dependencies) {
		if (!validate_relationship_cycle(
				dependency.first, dependencies, &visiting, &visited)) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidClosureDependency,
				"Closure dependency graph contains a cycle.", dependency.first);
			break;
		}
	}

	const auto glass_identifiers = collect_identifiers(
		architecture.dropGlassAssemblies(),
		[](const DropGlassAssembly &glass) -> const std::string & {
			return glass.identifier();
		},
		VehicleDiagnosticCode::InvalidGuideRailJoint, "DropGlassAssembly", &report);
	(void)glass_identifiers;
	for (const DropGlassAssembly &glass : architecture.dropGlassAssemblies()) {
		const GuideRailJoint &joint = glass.guideRailJoint();
		if (closure_identifiers.count(glass.parentClosureIdentifier()) == 0u ||
		    glass.glassPanelIdentifier().empty() || !normalized_value(glass.state()) ||
		    !valid_bounds(glass.doorInnerVolume()) || !joint.frontRail().isValid() ||
		    !joint.rearRail().isValid() ||
		    glm::distance(joint.frontFollower(), joint.rearFollower()) <= 1.0e-6f ||
		    !normalized_value(joint.lowerLimit()) || !normalized_value(joint.upperLimit()) ||
		    joint.lowerLimit() >= joint.upperLimit()) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidGuideRailJoint,
				"Drop glass requires an owning closure, valid rails, separated followers, valid cavity, and ordered normalized limits.",
				glass.identifier());
		}
		const ClosureAssembly *parent = architecture.findClosure(
			glass.parentClosureIdentifier());
		if (parent == nullptr || parent->closureType() != VehicleClosureType::SideDoor) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidClosureDependency,
				"Drop glass must be parented to a side-door closure.", glass.identifier());
		}
		if (joint.evidenceClassification() == VehicleEvidenceClassification::Observed) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidEvidenceClassification,
				"Hidden guide-rail mechanisms must not be classified as directly observed.",
				joint.identifier());
		}
	}

	for (float state : architecture.closureState().orderedValues()) {
		if (!normalized_value(state)) {
			add_issue(
				&report, VehicleDiagnosticCode::InvalidClosureAssembly,
				"Every VehicleClosureState value must be finite and within [0,1].");
		}
	}
	return report;
}
