#include "vehicle/service/RedAwdHatchbackMvpv2Builder.h"

#include "vehicle/service/ModernEvFastbackBuilder.h"
#include "vehicle/service/VehicleClosureKinematicsService.h"
#include "vehicle/service/VehicleFittingDeterministicHashService.h"
#include "vehicle/service/VehicleFittingValidationService.h"

#include <array>
#include <memory>
#include <utility>
#include <vector>

namespace {

VehicleViewEnvelope create_view_envelope(
	const std::string &identifier,
	VehicleReferenceView view,
	const std::array<glm::vec2, 5> &points,
	float weight)
{
	const char *semantic_names[] = {
		"lower-left", "upper-left", "apex", "upper-right", "lower-right"};
	std::vector<ViewSilhouetteSample> samples;
	for (std::size_t index = 0u; index < points.size(); ++index) {
		samples.emplace_back(semantic_names[index], points[index]);
	}
	return VehicleViewEnvelope(
		identifier, view, identifier + ".Camera", std::move(samples), weight);
}

VehicleCrossSection create_cross_section(
	const std::string &identifier,
	float station_z,
	float half_width,
	float roof_height,
	float greenhouse_width)
{
	return VehicleCrossSection(identifier, station_z, {
		{VehicleSectionLandmarkRole::CentreRoof, {0.0f, roof_height, station_z}},
		{VehicleSectionLandmarkRole::RoofRail,
			{greenhouse_width, roof_height - 0.05f, station_z}},
		{VehicleSectionLandmarkRole::GlassShoulder,
			{greenhouse_width + 0.10f, roof_height - 0.28f, station_z}},
		{VehicleSectionLandmarkRole::Belt,
			{half_width - 0.06f, 0.98f, station_z}},
		{VehicleSectionLandmarkRole::UpperShoulder,
			{half_width, 0.84f, station_z}},
		{VehicleSectionLandmarkRole::LowerDoor,
			{half_width - 0.02f, 0.50f, station_z}},
		{VehicleSectionLandmarkRole::Rocker,
			{half_width - 0.06f, 0.22f, station_z}},
		{VehicleSectionLandmarkRole::Underbody,
			{half_width * 0.52f, 0.15f, station_z}},
		{VehicleSectionLandmarkRole::CentreFloor, {0.0f, 0.15f, station_z}}});
}

SurfaceLandmark create_landmark(
	const std::string &identifier,
	glm::vec3 point,
	std::initializer_list<VehicleReferenceView> observed_views)
{
	std::vector<SurfaceLandmarkObservation> observations;
	float coordinate = 0.18f;
	for (VehicleReferenceView view : observed_views) {
		observations.emplace_back(view, glm::vec2(coordinate, 1.0f - coordinate), 1.0f);
		coordinate += 0.11f;
	}
	return SurfaceLandmark(
		identifier, point, std::move(observations), 0.92f,
		VehicleEvidenceClassification::Triangulated);
}

VehicleCharacterCurve create_character_curve(
	const std::string &identifier,
	VehicleCharacterCurveRole role,
	std::vector<glm::vec3> points,
	std::vector<std::string> landmarks = {})
{
	return VehicleCharacterCurve(
		identifier, role, Curve3D(Curve3DType::Polyline, std::move(points), identifier),
		std::move(landmarks), VehicleEvidenceClassification::Triangulated);
}

ProjectionConstrainedPatch create_patch(
	const std::string &identifier,
	std::array<std::string, 4> boundary_curves,
	std::vector<std::string> free_landmarks,
	std::initializer_list<VehicleReferenceView> views)
{
	std::vector<PatchProjectionConstraint> projections;
	for (VehicleReferenceView view : views) {
		projections.emplace_back(view, identifier + ".Silhouette", 1.0f, 0.012f);
	}
	return ProjectionConstrainedPatch(
		BoundaryPatch(
			identifier, std::move(boundary_curves), {},
			{PatchNormalConstraint(glm::vec3(0.0f, 1.0f, 0.0f), 0.25f)}),
		std::move(projections), {}, std::move(free_landmarks), 1.0f, 0.08f);
}

Curve3D create_closed_seam(
	const std::string &identifier,
	glm::vec3 minimum,
	glm::vec3 maximum)
{
	return Curve3D(Curve3DType::Polyline, {
		{minimum.x, minimum.y, minimum.z},
		{maximum.x, minimum.y, minimum.z},
		{maximum.x, maximum.y, maximum.z},
		{minimum.x, maximum.y, maximum.z},
		{minimum.x, minimum.y, minimum.z}}, identifier);
}

AxisAlignedBounds create_bounds(glm::vec3 minimum, glm::vec3 maximum)
{
	AxisAlignedBounds bounds;
	bounds.min = minimum;
	bounds.max = maximum;
	bounds.center = (minimum + maximum) * 0.5f;
	bounds.half_extents = (maximum - minimum) * 0.5f;
	bounds.valid = true;
	return bounds;
}

ClosureAssembly create_door_closure(
	const std::string &identifier,
	const std::string &aperture_identifier,
	const std::string &panel_identifier,
	float side,
	float front_z,
	float rear_z,
	float state)
{
	const HingePair hinge(
		identifier + ".HingePair",
		{side * 0.91f, 0.42f, front_z},
		{side * 0.91f, 1.14f, front_z},
		68.0f, VehicleEvidenceClassification::EngineeringInference);
	const AxisAlignedBounds panel_bounds = create_bounds(
		{std::min(side * 0.94f, side * 0.87f), 0.28f, rear_z},
		{std::max(side * 0.94f, side * 0.87f), 1.36f, front_z});
	const ClosureKinematicRelationship relationship = hinge;
	const SweptVolume swept = VehicleClosureKinematicsService().calculateSweptVolume(
		panel_bounds, relationship, 17u);
	return ClosureAssembly(
		identifier, VehicleClosureType::SideDoor, "MVPV2.BodyStructure",
		aperture_identifier, panel_identifier, panel_identifier + ".Inner",
		relationship, {identifier + ".PerimeterSeal"},
		{identifier + ".Latch"}, {identifier + ".Glass"}, {},
		VehicleViewEvidenceWeights(0.05f, 0.05f, 0.35f, 0.35f, 0.20f),
		state, swept);
}

DropGlassAssembly create_drop_glass(
	const std::string &identifier,
	const std::string &parent_closure,
	float side,
	float front_z,
	float rear_z,
	float state)
{
	const Curve3D front_rail(
		Curve3DType::Bezier,
		{{side * 0.88f, 0.42f, front_z}, {side * 0.89f, 0.72f, front_z - 0.01f},
		 {side * 0.87f, 1.02f, front_z - 0.03f}, {side * 0.83f, 1.30f, front_z - 0.06f}},
		identifier + ".FrontRail");
	const Curve3D rear_rail(
		Curve3DType::Bezier,
		{{side * 0.88f, 0.42f, rear_z}, {side * 0.89f, 0.72f, rear_z + 0.01f},
		 {side * 0.87f, 1.02f, rear_z + 0.02f}, {side * 0.82f, 1.27f, rear_z + 0.04f}},
		identifier + ".RearRail");
	return DropGlassAssembly(
		identifier, parent_closure, identifier + ".CurvedGlassPanel",
		GuideRailJoint(
			identifier + ".GuideRailJoint", front_rail, rear_rail,
			{side * 0.84f, 1.28f, front_z - 0.05f},
			{side * 0.83f, 1.25f, rear_z + 0.03f},
			0.0f, 1.0f, VehicleEvidenceClassification::EngineeringInference),
		BeltSeal(identifier + ".InnerBeltSeal", identifier + ".BeltLine", 0.012f, 0.018f),
		BeltSeal(identifier + ".OuterBeltSeal", identifier + ".BeltLine", 0.012f, 0.018f),
		create_bounds(
			{std::min(side * 0.94f, side * 0.80f), 0.20f, rear_z - 0.04f},
			{std::max(side * 0.94f, side * 0.80f), 1.34f, front_z + 0.04f}),
		state);
}

} // namespace

VehicleFittingArchitecture RedAwdHatchbackMvpv2Builder::createFittingArchitecture() const
{
	MultiViewEnvelope envelope(
		"MVPV2.RedAwdHatchback.MultiViewEnvelope",
		{
			create_view_envelope("Front", VehicleReferenceView::Front,
				{{{0.08f, 0.78f}, {0.15f, 0.32f}, {0.50f, 0.10f}, {0.85f, 0.32f}, {0.92f, 0.78f}}}, 0.20f),
			create_view_envelope("Rear", VehicleReferenceView::Rear,
				{{{0.07f, 0.76f}, {0.16f, 0.30f}, {0.50f, 0.12f}, {0.84f, 0.30f}, {0.93f, 0.76f}}}, 0.20f),
			create_view_envelope("Left", VehicleReferenceView::Left,
				{{{0.04f, 0.80f}, {0.19f, 0.37f}, {0.53f, 0.14f}, {0.87f, 0.40f}, {0.96f, 0.79f}}}, 0.20f),
			create_view_envelope("Right", VehicleReferenceView::Right,
				{{{0.04f, 0.79f}, {0.18f, 0.39f}, {0.52f, 0.14f}, {0.86f, 0.38f}, {0.96f, 0.80f}}}, 0.20f),
			create_view_envelope("Top", VehicleReferenceView::Top,
				{{{0.06f, 0.50f}, {0.20f, 0.18f}, {0.50f, 0.10f}, {0.82f, 0.20f}, {0.94f, 0.50f}}}, 0.20f)},
		0.02f);

	std::vector<VehicleCrossSection> sections = {
		create_cross_section("RearBumper", -2.20f, 0.72f, 0.76f, 0.28f),
		create_cross_section("RearQuarter", -1.70f, 0.91f, 1.34f, 0.64f),
		create_cross_section("RearDoor", -0.75f, 0.92f, 1.44f, 0.68f),
		create_cross_section("B-Pillar", 0.18f, 0.91f, 1.40f, 0.66f),
		create_cross_section("FrontDoor", 0.78f, 0.90f, 1.15f, 0.56f),
		create_cross_section("FrontFender", 1.45f, 0.84f, 0.92f, 0.40f),
		create_cross_section("FrontBumper", 2.14f, 0.70f, 0.74f, 0.26f)};

	std::vector<SurfaceLandmark> landmarks = {
		create_landmark("RoofFront", {0.0f, 1.34f, 0.78f}, {VehicleReferenceView::Left, VehicleReferenceView::Top}),
		create_landmark("RoofApex", {0.0f, 1.44f, -0.30f}, {VehicleReferenceView::Left, VehicleReferenceView::Front}),
		create_landmark("RoofRear", {0.0f, 1.29f, -1.50f}, {VehicleReferenceView::Left, VehicleReferenceView::Rear}),
		create_landmark("HoodFront", {0.0f, 0.76f, 2.05f}, {VehicleReferenceView::Left, VehicleReferenceView::Top}),
		create_landmark("HoodRear", {0.0f, 0.96f, 0.90f}, {VehicleReferenceView::Left, VehicleReferenceView::Top}),
		create_landmark("BeltFrontL", {-0.88f, 0.98f, 1.55f}, {VehicleReferenceView::Left, VehicleReferenceView::Front}),
		create_landmark("BeltRearL", {-0.89f, 1.03f, -1.72f}, {VehicleReferenceView::Left, VehicleReferenceView::Rear}),
		create_landmark("BeltFrontR", {0.88f, 0.98f, 1.55f}, {VehicleReferenceView::Right, VehicleReferenceView::Front}),
		create_landmark("BeltRearR", {0.89f, 1.03f, -1.72f}, {VehicleReferenceView::Right, VehicleReferenceView::Rear}),
		create_landmark("FrontArchApexL", {-0.91f, 0.82f, 0.92f}, {VehicleReferenceView::Left}),
		create_landmark("RearArchApexL", {-0.91f, 0.82f, -1.70f}, {VehicleReferenceView::Left}),
		create_landmark("FrontArchApexR", {0.91f, 0.82f, 0.92f}, {VehicleReferenceView::Right}),
		create_landmark("RearArchApexR", {0.91f, 0.82f, -1.70f}, {VehicleReferenceView::Right}),
		create_landmark("SpoilerEdge", {0.0f, 1.31f, -2.08f}, {VehicleReferenceView::Rear, VehicleReferenceView::Top})};

	std::vector<VehicleCharacterCurve> curves = {
		create_character_curve("RoofLine", VehicleCharacterCurveRole::RoofLine,
			{{0, 1.34f, 0.78f}, {0, 1.44f, -0.30f}, {0, 1.29f, -1.50f}}, {"RoofFront", "RoofApex", "RoofRear"}),
		create_character_curve("HoodCentre", VehicleCharacterCurveRole::HoodCentre,
			{{0, 0.76f, 2.05f}, {0, 0.90f, 1.45f}, {0, 0.96f, 0.90f}}, {"HoodFront", "HoodRear"}),
		create_character_curve("HoodOuterEdgeL", VehicleCharacterCurveRole::HoodOuterEdge,
			{{-0.68f, 0.74f, 2.05f}, {-0.77f, 0.91f, 0.90f}}),
		create_character_curve("HoodOuterEdgeR", VehicleCharacterCurveRole::HoodOuterEdge,
			{{0.68f, 0.74f, 2.05f}, {0.77f, 0.91f, 0.90f}}),
		create_character_curve("BeltLineL", VehicleCharacterCurveRole::BeltLine,
			{{-0.88f, 0.98f, 1.55f}, {-0.91f, 1.00f, 0.0f}, {-0.89f, 1.03f, -1.72f}}, {"BeltFrontL", "BeltRearL"}),
		create_character_curve("BeltLineR", VehicleCharacterCurveRole::BeltLine,
			{{0.88f, 0.98f, 1.55f}, {0.91f, 1.00f, 0.0f}, {0.89f, 1.03f, -1.72f}}, {"BeltFrontR", "BeltRearR"}),
		create_character_curve("UpperShoulderL", VehicleCharacterCurveRole::UpperShoulder,
			{{-0.80f, 0.87f, 1.90f}, {-0.92f, 0.87f, 0.0f}, {-0.91f, 0.91f, -1.95f}}),
		create_character_curve("UpperShoulderR", VehicleCharacterCurveRole::UpperShoulder,
			{{0.80f, 0.87f, 1.90f}, {0.92f, 0.87f, 0.0f}, {0.91f, 0.91f, -1.95f}}),
		create_character_curve("LowerDoorCreaseL", VehicleCharacterCurveRole::LowerDoorCrease,
			{{-0.90f, 0.48f, 1.45f}, {-0.92f, 0.44f, -1.50f}}),
		create_character_curve("LowerDoorCreaseR", VehicleCharacterCurveRole::LowerDoorCrease,
			{{0.90f, 0.48f, 1.45f}, {0.92f, 0.44f, -1.50f}}),
		create_character_curve("RockerLineL", VehicleCharacterCurveRole::RockerLine,
			{{-0.86f, 0.22f, 1.55f}, {-0.87f, 0.22f, -1.70f}}),
		create_character_curve("RockerLineR", VehicleCharacterCurveRole::RockerLine,
			{{0.86f, 0.22f, 1.55f}, {0.87f, 0.22f, -1.70f}}),
		create_character_curve("WindowUpperLineL", VehicleCharacterCurveRole::WindowUpperLine,
			{{-0.72f, 1.25f, 0.74f}, {-0.68f, 1.40f, -0.30f}, {-0.66f, 1.26f, -1.48f}}),
		create_character_curve("WindowUpperLineR", VehicleCharacterCurveRole::WindowUpperLine,
			{{0.72f, 1.25f, 0.74f}, {0.68f, 1.40f, -0.30f}, {0.66f, 1.26f, -1.48f}}),
		create_character_curve("FrontFenderCrestL", VehicleCharacterCurveRole::FrontFenderCrest,
			{{-0.88f, 0.74f, 1.45f}, {-0.91f, 0.82f, 0.92f}, {-0.88f, 0.72f, 0.42f}}, {"FrontArchApexL"}),
		create_character_curve("FrontFenderCrestR", VehicleCharacterCurveRole::FrontFenderCrest,
			{{0.88f, 0.74f, 1.45f}, {0.91f, 0.82f, 0.92f}, {0.88f, 0.72f, 0.42f}}, {"FrontArchApexR"}),
		create_character_curve("RearHaunchL", VehicleCharacterCurveRole::RearHaunch,
			{{-0.89f, 0.78f, -1.20f}, {-0.91f, 0.82f, -1.70f}, {-0.82f, 0.75f, -2.10f}}, {"RearArchApexL"}),
		create_character_curve("RearHaunchR", VehicleCharacterCurveRole::RearHaunch,
			{{0.89f, 0.78f, -1.20f}, {0.91f, 0.82f, -1.70f}, {0.82f, 0.75f, -2.10f}}, {"RearArchApexR"})};

	CharacterCurveNetwork curve_network(std::move(curves), {
		{"HoodCentre", "RoofLine", CharacterCurveRelationshipType::TerminatesAt, 0.02f},
		{"BeltLineL", "RearHaunchL", CharacterCurveRelationshipType::Continues, 0.02f},
		{"BeltLineR", "RearHaunchR", CharacterCurveRelationshipType::Continues, 0.02f}});

	std::vector<ProjectionConstrainedPatch> patches = {
		create_patch("HoodPatch", {"HoodOuterEdgeL", "HoodCentre", "HoodOuterEdgeR", "UpperShoulderL"}, {"HoodFront", "HoodRear"}, {VehicleReferenceView::Top, VehicleReferenceView::Front, VehicleReferenceView::Left}),
		create_patch("FenderPatchL", {"HoodOuterEdgeL", "FrontFenderCrestL", "BeltLineL", "UpperShoulderL"}, {"FrontArchApexL"}, {VehicleReferenceView::Left, VehicleReferenceView::Top}),
		create_patch("FenderPatchR", {"HoodOuterEdgeR", "FrontFenderCrestR", "BeltLineR", "UpperShoulderR"}, {"FrontArchApexR"}, {VehicleReferenceView::Right, VehicleReferenceView::Top}),
		create_patch("FrontDoorPatchL", {"BeltLineL", "UpperShoulderL", "LowerDoorCreaseL", "RockerLineL"}, {"BeltFrontL"}, {VehicleReferenceView::Left, VehicleReferenceView::Top}),
		create_patch("FrontDoorPatchR", {"BeltLineR", "UpperShoulderR", "LowerDoorCreaseR", "RockerLineR"}, {"BeltFrontR"}, {VehicleReferenceView::Right, VehicleReferenceView::Top}),
		create_patch("RearDoorPatchL", {"BeltLineL", "WindowUpperLineL", "LowerDoorCreaseL", "RockerLineL"}, {"BeltRearL"}, {VehicleReferenceView::Left, VehicleReferenceView::Top}),
		create_patch("RearDoorPatchR", {"BeltLineR", "WindowUpperLineR", "LowerDoorCreaseR", "RockerLineR"}, {"BeltRearR"}, {VehicleReferenceView::Right, VehicleReferenceView::Top}),
		create_patch("RoofPatch", {"WindowUpperLineL", "RoofLine", "WindowUpperLineR", "HoodCentre"}, {"RoofApex"}, {VehicleReferenceView::Top, VehicleReferenceView::Left, VehicleReferenceView::Right}),
		create_patch("QuarterPatchL", {"RearHaunchL", "BeltLineL", "WindowUpperLineL", "RockerLineL"}, {"RearArchApexL"}, {VehicleReferenceView::Left, VehicleReferenceView::Rear}),
		create_patch("QuarterPatchR", {"RearHaunchR", "BeltLineR", "WindowUpperLineR", "RockerLineR"}, {"RearArchApexR"}, {VehicleReferenceView::Right, VehicleReferenceView::Rear}),
		create_patch("HatchPatch", {"RoofLine", "RearHaunchL", "RearHaunchR", "BeltLineL"}, {"RoofRear", "SpoilerEdge"}, {VehicleReferenceView::Rear, VehicleReferenceView::Top, VehicleReferenceView::Left}),
		create_patch("FrontBumperPatch", {"HoodOuterEdgeL", "HoodCentre", "HoodOuterEdgeR", "FrontFenderCrestL"}, {"HoodFront"}, {VehicleReferenceView::Front, VehicleReferenceView::Top}),
		create_patch("RearBumperPatch", {"RearHaunchL", "RoofLine", "RearHaunchR", "RockerLineL"}, {"SpoilerEdge"}, {VehicleReferenceView::Rear, VehicleReferenceView::Top})};

	SurfacePatchGraph patch_graph(std::move(patches), {
		{"HoodPatch", "left", "FenderPatchL", "hood", SurfaceEdgeRelationshipType::TangentG1, 0.01f},
		{"HoodPatch", "right", "FenderPatchR", "hood", SurfaceEdgeRelationshipType::TangentG1, 0.01f},
		{"FenderPatchL", "rear", "FrontDoorPatchL", "front", SurfaceEdgeRelationshipType::ShutLine, 0.004f},
		{"FrontDoorPatchL", "rear", "RearDoorPatchL", "front", SurfaceEdgeRelationshipType::ShutLine, 0.004f},
		{"RearDoorPatchL", "rear", "QuarterPatchL", "front", SurfaceEdgeRelationshipType::ShutLine, 0.004f},
		{"QuarterPatchL", "roof", "RoofPatch", "left", SurfaceEdgeRelationshipType::TangentG1, 0.01f},
		{"QuarterPatchR", "roof", "RoofPatch", "right", SurfaceEdgeRelationshipType::TangentG1, 0.01f},
		{"RoofPatch", "rear", "HatchPatch", "header", SurfaceEdgeRelationshipType::Aperture, 0.006f}});

	std::vector<PanelSeamLoop> seams = {
		{"DoorFL.Seam", "FrontDoorPatchL", create_closed_seam("DoorFL.SeamCurve", {-0.93f, 0.24f, -0.30f}, {-0.86f, 1.36f, 0.72f}), 0.004f, 0.008f},
		{"DoorFR.Seam", "FrontDoorPatchR", create_closed_seam("DoorFR.SeamCurve", {0.86f, 0.24f, -0.30f}, {0.93f, 1.36f, 0.72f}), 0.004f, 0.008f},
		{"DoorRL.Seam", "RearDoorPatchL", create_closed_seam("DoorRL.SeamCurve", {-0.93f, 0.24f, -1.35f}, {-0.86f, 1.36f, -0.34f}), 0.004f, 0.008f},
		{"DoorRR.Seam", "RearDoorPatchR", create_closed_seam("DoorRR.SeamCurve", {0.86f, 0.24f, -1.35f}, {0.93f, 1.36f, -0.34f}), 0.004f, 0.008f},
		{"Bonnet.Seam", "HoodPatch", create_closed_seam("Bonnet.SeamCurve", {-0.76f, 0.73f, 0.88f}, {0.76f, 0.98f, 2.08f}), 0.004f, 0.010f},
		{"RearHatch.Seam", "HatchPatch", create_closed_seam("RearHatch.SeamCurve", {-0.72f, 0.56f, -2.16f}, {0.72f, 1.31f, -1.43f}), 0.005f, 0.012f}};

	std::vector<ExtractedSurfacePanel> panels = {
		{"DoorFL.OuterPanel", "FrontDoorPatchL", "DoorFL.Seam"},
		{"DoorFR.OuterPanel", "FrontDoorPatchR", "DoorFR.Seam"},
		{"DoorRL.OuterPanel", "RearDoorPatchL", "DoorRL.Seam"},
		{"DoorRR.OuterPanel", "RearDoorPatchR", "DoorRR.Seam"},
		{"Bonnet.OuterPanel", "HoodPatch", "Bonnet.Seam"},
		{"RearHatch.OuterPanel", "HatchPatch", "RearHatch.Seam"}};

	std::vector<Aperture> apertures = {
		{"DoorFL.Aperture", "MVPV2.BodyStructure", "DoorFL.Seam", "DoorFL.InnerBoundary", 0.032f, "DoorFL.SealSeat", {"A-PillarL", "B-PillarL"}, {"DoorFL.HingeMounts", "DoorFL.StrikerMount"}},
		{"DoorFR.Aperture", "MVPV2.BodyStructure", "DoorFR.Seam", "DoorFR.InnerBoundary", 0.032f, "DoorFR.SealSeat", {"A-PillarR", "B-PillarR"}, {"DoorFR.HingeMounts", "DoorFR.StrikerMount"}},
		{"DoorRL.Aperture", "MVPV2.BodyStructure", "DoorRL.Seam", "DoorRL.InnerBoundary", 0.032f, "DoorRL.SealSeat", {"B-PillarL", "C-PillarL"}, {"DoorRL.HingeMounts", "DoorRL.StrikerMount"}},
		{"DoorRR.Aperture", "MVPV2.BodyStructure", "DoorRR.Seam", "DoorRR.InnerBoundary", 0.032f, "DoorRR.SealSeat", {"B-PillarR", "C-PillarR"}, {"DoorRR.HingeMounts", "DoorRR.StrikerMount"}},
		{"Bonnet.Aperture", "MVPV2.BodyStructure", "Bonnet.Seam", "EngineBay.InnerBoundary", 0.026f, "Bonnet.SealSeat", {"CowlCrossmember"}, {"Bonnet.HingeMounts", "Bonnet.LatchMount"}},
		{"RearHatch.Aperture", "MVPV2.BodyStructure", "RearHatch.Seam", "RearHatch.InnerBoundary", 0.030f, "RearHatch.SealSeat", {"RoofHeader", "RearSill"}, {"RearHatch.HingeMounts", "RearHatch.StrikerMount"}}};

	VehicleClosureState state(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f);
	std::vector<ClosureAssembly> closures;
	closures.push_back(create_door_closure("DoorFL", "DoorFL.Aperture", "DoorFL.OuterPanel", -1.0f, 0.72f, -0.30f, state.frontLeftDoor()));
	closures.push_back(create_door_closure("DoorFR", "DoorFR.Aperture", "DoorFR.OuterPanel", 1.0f, 0.72f, -0.30f, state.frontRightDoor()));
	closures.push_back(create_door_closure("DoorRL", "DoorRL.Aperture", "DoorRL.OuterPanel", -1.0f, -0.34f, -1.35f, state.rearLeftDoor()));
	closures.push_back(create_door_closure("DoorRR", "DoorRR.Aperture", "DoorRR.OuterPanel", 1.0f, -0.34f, -1.35f, state.rearRightDoor()));

	const ClosureKinematicRelationship bonnet_joint = FourBarJoint(
		"Bonnet.FourBarJoint", {-0.64f, 0.93f, 0.88f}, {0.64f, 0.93f, 0.88f},
		{-0.58f, 0.94f, 0.96f}, {0.58f, 0.94f, 0.96f}, 68.0f,
		{0.0f, 0.08f, -0.05f}, VehicleEvidenceClassification::EngineeringInference);
	const AxisAlignedBounds bonnet_bounds = create_bounds({-0.76f, 0.72f, 0.88f}, {0.76f, 0.99f, 2.08f});
	closures.emplace_back(
		"Bonnet", VehicleClosureType::Bonnet, "MVPV2.BodyStructure",
		"Bonnet.Aperture", "Bonnet.OuterPanel", "Bonnet.InnerPanel", bonnet_joint,
		std::vector<std::string>{"Bonnet.PerimeterSeal"},
		std::vector<std::string>{"Bonnet.PrimaryLatch", "Bonnet.SafetyLatch"},
		std::vector<std::string>{},
		std::vector<TelescopingLink>{TelescopingLink(
			"Bonnet.SupportStrut", {-0.55f, 0.72f, 1.10f}, {-0.48f, 0.88f, 1.55f},
			0.42f, 0.76f, VehicleEvidenceClassification::EngineeringInference)},
		VehicleViewEvidenceWeights(0.30f, 0.0f, 0.15f, 0.15f, 0.40f),
		state.bonnet(), VehicleClosureKinematicsService().calculateSweptVolume(
			bonnet_bounds, bonnet_joint, 17u));

	const ClosureKinematicRelationship hatch_joint = FourBarJoint(
		"RearHatch.FourBarJoint", {-0.58f, 1.28f, -1.48f}, {0.58f, 1.28f, -1.48f},
		{-0.55f, 1.26f, -1.54f}, {0.55f, 1.26f, -1.54f}, 82.0f,
		{0.0f, 0.11f, 0.03f}, VehicleEvidenceClassification::EngineeringInference);
	const AxisAlignedBounds hatch_bounds = create_bounds({-0.72f, 0.54f, -2.18f}, {0.72f, 1.33f, -1.42f});
	closures.emplace_back(
		"RearHatch", VehicleClosureType::RearHatch, "MVPV2.BodyStructure",
		"RearHatch.Aperture", "RearHatch.OuterPanel", "RearHatch.InnerPanel",
		hatch_joint, std::vector<std::string>{"RearHatch.PerimeterSeal"},
		std::vector<std::string>{"RearHatch.Latch"},
		std::vector<std::string>{"RearGlass", "RearSpoiler", "HighBrakeLight"},
		std::vector<TelescopingLink>{
			TelescopingLink("RearHatch.StrutL", {-0.62f, 1.10f, -1.56f}, {-0.58f, 0.90f, -1.82f}, 0.38f, 0.72f, VehicleEvidenceClassification::EngineeringInference),
			TelescopingLink("RearHatch.StrutR", {0.62f, 1.10f, -1.56f}, {0.58f, 0.90f, -1.82f}, 0.38f, 0.72f, VehicleEvidenceClassification::EngineeringInference)},
		VehicleViewEvidenceWeights(0.0f, 0.45f, 0.15f, 0.15f, 0.25f),
		state.hatch(), VehicleClosureKinematicsService().calculateSweptVolume(
			hatch_bounds, hatch_joint, 17u));

	std::vector<DropGlassAssembly> glass = {
		create_drop_glass("WindowFL", "DoorFL", -1.0f, 0.66f, -0.24f, state.frontLeftWindow()),
		create_drop_glass("WindowFR", "DoorFR", 1.0f, 0.66f, -0.24f, state.frontRightWindow()),
		create_drop_glass("WindowRL", "DoorRL", -1.0f, -0.40f, -1.29f, state.rearLeftWindow()),
		create_drop_glass("WindowRR", "DoorRR", 1.0f, -0.40f, -1.29f, state.rearRightWindow())};

	return VehicleFittingArchitecture(
		"MVPV2.RedAwdHatchback", std::move(envelope), std::move(sections),
		std::move(landmarks), std::move(curve_network), std::move(patch_graph),
		std::move(seams), std::move(panels), std::move(apertures),
		std::move(closures), std::move(glass), state);
}

std::optional<ModernVehicleAssembly> RedAwdHatchbackMvpv2Builder::build(
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	std::optional<ModernVehicleAssembly> base_vehicle =
		ModernEvFastbackBuilder().buildAcceptanceVehicle(detail_level, diagnostic);
	if (!base_vehicle.has_value()) return std::nullopt;

	auto architecture = std::make_shared<const VehicleFittingArchitecture>(
		createFittingArchitecture());
	const VehicleValidationReport validation =
		VehicleFittingValidationService().validate(*architecture);
	if (!validation.isValid()) {
		if (diagnostic != nullptr) {
			*diagnostic = std::string(vehicleDiagnosticCodeName(
				validation.issues().front().code())) + ": " +
				validation.issues().front().message();
		}
		return std::nullopt;
	}
	const std::uint64_t fitting_hash =
		VehicleFittingDeterministicHashService().calculate(*architecture);
	return base_vehicle->withFittingArchitecture(
		std::move(architecture), fitting_hash, validation);
}
