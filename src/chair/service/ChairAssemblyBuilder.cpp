#include "chair/service/ChairAssemblyBuilder.h"

#include "geometry/model/CompoundShapeSpecification.h"
#include "geometry/model/CurveNetworkSurfaceShapeSpecification.h"
#include "geometry/model/MirrorCurve3D.h"
#include "geometry/model/ShellOffsetShapeSpecification.h"
#include "geometry/service/CompoundShapeSpecificationValidator.h"
#include "geometry/service/CurveNetworkSurfaceSpecificationValidator.h"
#include "geometry/service/SweepDiskSpecificationValidator.h"

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <iomanip>
#include <sstream>

namespace {

std::shared_ptr<const ShapeSpecification> swept_member(
	const std::string &purpose,
	std::vector<glm::vec3> points,
	float start_radius,
	float end_radius,
	std::string *diagnostic)
{
	(void)purpose;
	SweepDiskShapeSpecificationCandidate candidate;
	candidate.curve_type = points.size() == 2u
		? Curve3DType::Line : Curve3DType::Polyline;
	candidate.curve_control_points = std::move(points);
	candidate.curve_was_explicit = true;
	candidate.radius_start = start_radius;
	candidate.radius_end = end_radius;
	candidate.radius_start_was_explicit = true;
	candidate.radius_end_was_explicit = true;
	candidate.longitudinal_segments = 16;
	candidate.circumferential_segments = 12;
	candidate.up_hint = glm::vec3(0.0f, 1.0f, 0.0f);
	const auto shape = SweepDiskSpecificationValidator().validate(
		std::move(candidate), diagnostic);
	return std::static_pointer_cast<const ShapeSpecification>(shape);
}

Curve3D mirrored_polyline(const Curve3D &curve)
{
	auto source = std::make_shared<const Curve3D>(curve);
	MirrorCurve3D mirrored(source, MirrorCurvePlaneAxis::X);
	std::vector<glm::vec3> points;
	for (std::size_t index = 0u; index < 4u; ++index) {
		points.emplace_back(mirrored.evaluatePosition(static_cast<double>(index) / 3.0));
	}
	return Curve3D(Curve3DType::Polyline, std::move(points), "Chair.MirroredShellEdge");
}

std::shared_ptr<const ShapeSpecification> shell_component(
	const std::string &purpose,
	std::vector<Curve3D> constant_u_curves,
	std::vector<Curve3D> constant_v_curves,
	float thickness,
	int samples_u,
	int samples_v,
	std::string *diagnostic)
{
	CurveNetworkSurfaceShapeSpecificationCandidate surface_candidate;
	surface_candidate.constant_u_curves = std::move(constant_u_curves);
	surface_candidate.constant_v_curves = std::move(constant_v_curves);
	surface_candidate.samples_u = samples_u;
	surface_candidate.samples_v = samples_v;
	surface_candidate.intersection_tolerance = 1.0e-4;
	auto surface = CurveNetworkSurfaceSpecificationValidator().validate(
		std::move(surface_candidate), diagnostic);
	if (!surface) return {};
	std::ostringstream key;
	key << "ShellOffset:COMv1:" << purpose << ":source="
	    << surface->key().canonicalValue() << ":thickness=" << thickness;
	std::ostringstream grammar;
	grammar << "ShellOffset(source(" << surface->canonicalText() << ") thickness("
	        << thickness << ") side(both))";
	return std::make_shared<const ShellOffsetShapeSpecification>(
		std::static_pointer_cast<const ShapeSpecification>(surface),
		thickness,
		ShellOffsetSide::Both,
		ShapeSpecificationKey(key.str()),
		grammar.str());
}

void add_component(
	std::vector<ChairComponent> *components,
	std::vector<CompoundShapePartCandidate> *parts,
	std::string purpose,
	ChairComponentCategory category,
	std::shared_ptr<const ShapeSpecification> shape)
{
	components->emplace_back(purpose, category, shape);
	parts->push_back({std::move(purpose), std::move(shape), glm::mat4(1.0f)});
}

std::string compound_grammar(const std::vector<ChairComponent> &components)
{
	std::ostringstream grammar;
	grammar << "CompoundShape(";
	for (const ChairComponent &component : components) {
		grammar << "part(" << component.purpose() << " source("
		        << component.shape()->canonicalText() << ")) ";
	}
	grammar << ")";
	return grammar.str();
}

std::string placement_grammar(const ChairPlacement &placement)
{
	const glm::vec3 &position = placement.groundContactPosition();
	std::ostringstream grammar;
	grammar << std::setprecision(7)
	        << "T(" << position.x << " " << position.y << " " << position.z << ") "
	        << "A(" << placement.orientation().yawDegrees() << " 1) "
	        << "A(" << placement.orientation().localToWorldPitchDegrees() << " 0)";
	if (std::fabs(placement.localGroundHeight()) > 1.0e-7f) {
		grammar << " T(0 0 " << -placement.localGroundHeight() << ")";
	}
	return grammar.str();
}

}

ChairObjectModelBuildResult ChairAssemblyBuilder::build(
	const ChairDesignDefinition &definition) const
{
	return build(definition, ChairPlacement::atWorldOrigin());
}

ChairObjectModelBuildResult ChairAssemblyBuilder::build(
	const ChairDesignDefinition &definition,
	const ChairPlacement &placement) const
{
	if (!placement.isValid()) {
		return ChairObjectModelBuildResult::failure(
			"Chair placement requires finite ground position, orientation, and local ground height.");
	}
	const ChairDimensionSpecification &dimensions = definition.dimensions();
	const ChairGeometryParameterSet &geometry = definition.geometry();
	const float half_width = dimensions.width() * 0.5f;
	const float half_depth = dimensions.depth() * 0.5f;
	const float seat_z = dimensions.seatHeight();
	std::string diagnostic;
	std::vector<ChairComponent> components;
	std::vector<CompoundShapePartCandidate> parts;

	const Curve3D left_seat_edge(
		Curve3DType::Bezier,
		{{-half_width, -half_depth, seat_z},
		 {-half_width, -half_depth * 0.30f, seat_z - geometry.shellCamber()},
		 {-half_width, half_depth * 0.30f, seat_z - geometry.shellCamber()},
		 {-half_width, half_depth, seat_z}},
		definition.identifier() + ".Seat.LeftEdge");
	const Curve3D right_seat_edge = mirrored_polyline(left_seat_edge);
	std::vector<Curve3D> seat_u{left_seat_edge, right_seat_edge};
	std::vector<Curve3D> seat_v{
		Curve3D(Curve3DType::Line,
			{{-half_width, -half_depth, seat_z}, {half_width, -half_depth, seat_z}},
			definition.identifier() + ".Seat.FrontEdge"),
		Curve3D(Curve3DType::Line,
			{{-half_width, half_depth, seat_z}, {half_width, half_depth, seat_z}},
			definition.identifier() + ".Seat.RearEdge")};
	auto seat = shell_component(
		"SeatShell", std::move(seat_u), std::move(seat_v),
		geometry.shellThickness(), 20, 20, &diagnostic);
	if (!seat) return ChairObjectModelBuildResult::failure(diagnostic);
	add_component(&components, &parts, "SeatShell", ChairComponentCategory::Seat, seat);

	if (geometry.hasBackrest()) {
		const float back_bottom_z = seat_z;
		const float back_top_z = dimensions.overallHeight();
		const float rear_y = half_depth;
		const float recline_shift = std::tan(glm::radians(
			definition.ergonomics().backReclineDegrees())) * (back_top_z - back_bottom_z);
		const Curve3D left_back_edge(
			Curve3DType::Bezier,
			{{-half_width, rear_y, back_bottom_z},
			 {-half_width, rear_y + recline_shift * 0.25f, back_bottom_z + (back_top_z - back_bottom_z) * 0.33f},
			 {-half_width, rear_y + recline_shift * 0.65f, back_bottom_z + (back_top_z - back_bottom_z) * 0.66f},
			 {-half_width, rear_y + recline_shift, back_top_z}},
			definition.identifier() + ".Back.LeftEdge");
		const Curve3D right_back_edge = mirrored_polyline(left_back_edge);
		std::vector<Curve3D> back_u{left_back_edge, right_back_edge};
		std::vector<Curve3D> back_v{
			Curve3D(Curve3DType::Line,
				{{-half_width, rear_y, back_bottom_z}, {half_width, rear_y, back_bottom_z}},
				definition.identifier() + ".Back.BottomEdge"),
			Curve3D(Curve3DType::Bezier,
				{{-half_width, rear_y + recline_shift, back_top_z},
				 {-half_width * 0.3f, rear_y + recline_shift - geometry.shellCamber(), back_top_z},
				 {half_width * 0.3f, rear_y + recline_shift - geometry.shellCamber(), back_top_z},
				 {half_width, rear_y + recline_shift, back_top_z}},
				definition.identifier() + ".Back.TopEdge")};
		auto back = shell_component(
			"BackrestShell", std::move(back_u), std::move(back_v),
			geometry.shellThickness(), 20, 24, &diagnostic);
		if (!back) return ChairObjectModelBuildResult::failure(diagnostic);
		add_component(&components, &parts, "BackrestShell", ChairComponentCategory::Backrest, back);
	}

	const float inset_x = half_width * 0.78f;
	const float inset_y = half_depth * 0.72f;
	const float radius = geometry.memberRadius();
	if (geometry.supportKind() == ChairSupportKind::FourLeg) {
		const glm::vec2 corners[] = {
			{-inset_x, -inset_y}, {inset_x, -inset_y},
			{-inset_x, inset_y}, {inset_x, inset_y}};
		for (std::size_t index = 0u; index < 4u; ++index) {
			const float splay_x = corners[index].x * 1.10f;
			const float splay_y = corners[index].y * 1.10f;
			auto leg = swept_member(
				"Leg", {{splay_x, splay_y, 0.0f},
				         {corners[index].x, corners[index].y, seat_z - 0.01f}},
				radius * 0.82f, radius, &diagnostic);
			if (!leg) return ChairObjectModelBuildResult::failure(diagnostic);
			add_component(&components, &parts, "Leg" + std::to_string(index + 1u),
				ChairComponentCategory::Support, leg);
		}
	}
	else if (geometry.supportKind() == ChairSupportKind::Cantilever ||
	         geometry.supportKind() == ChairSupportKind::Sled) {
		for (int side : {-1, 1}) {
			const float x = static_cast<float>(side) * inset_x;
			std::vector<glm::vec3> path = geometry.supportKind() == ChairSupportKind::Cantilever
				? std::vector<glm::vec3>{{x, inset_y, seat_z}, {x, inset_y, 0.04f},
				                         {x, -inset_y, 0.04f}, {x, -inset_y, seat_z * 0.72f}}
				: std::vector<glm::vec3>{{x, -inset_y, seat_z}, {x, -inset_y, 0.04f},
				                         {x, inset_y, 0.04f}, {x, inset_y, seat_z}};
			auto rail = swept_member("SupportRail", std::move(path), radius, radius, &diagnostic);
			if (!rail) return ChairObjectModelBuildResult::failure(diagnostic);
			add_component(&components, &parts,
				side < 0 ? "LeftSupportRail" : "RightSupportRail",
				ChairComponentCategory::Support, rail);
		}
	}
	else {
		auto column = swept_member(
			"PedestalColumn", {{0.0f, 0.0f, 0.05f}, {0.0f, 0.0f, seat_z}},
			radius * 1.35f, radius, &diagnostic);
		if (!column) return ChairObjectModelBuildResult::failure(diagnostic);
		add_component(&components, &parts, "PedestalColumn", ChairComponentCategory::Support, column);
		const int spoke_count = geometry.supportKind() == ChairSupportKind::SwivelCaster ? 5 : 4;
		for (int index = 0; index < spoke_count; ++index) {
			const float angle = glm::two_pi<float>() * static_cast<float>(index) /
				static_cast<float>(spoke_count);
			const glm::vec3 spoke_start(
				std::cos(angle) * radius * 1.8f,
				std::sin(angle) * radius * 1.8f,
				0.05f);
			auto spoke = swept_member(
				"BaseSpoke", {spoke_start,
				              {std::cos(angle) * half_width * 0.72f,
				               std::sin(angle) * half_depth * 0.72f, 0.035f}},
				radius, radius * 0.65f, &diagnostic);
			if (!spoke) return ChairObjectModelBuildResult::failure(diagnostic);
			add_component(&components, &parts, "BaseSpoke" + std::to_string(index + 1),
				ChairComponentCategory::Support, spoke);
		}
	}

	if (geometry.hasArmrests()) {
		for (int side : {-1, 1}) {
			const float x = static_cast<float>(side) * (half_width + radius * 0.6f);
			auto arm = swept_member(
				"Armrest", {{x, -half_depth * 0.60f, seat_z + 0.16f},
				             {x, half_depth * 0.55f, seat_z + 0.18f}},
				radius, radius, &diagnostic);
			if (!arm) return ChairObjectModelBuildResult::failure(diagnostic);
			add_component(&components, &parts,
				side < 0 ? "LeftArmrest" : "RightArmrest",
				ChairComponentCategory::Armrest, arm);
		}
	}
	if (geometry.hasFootrest()) {
		for (int side : {-1, 1}) {
			auto footrest = swept_member(
				"Footrest", {{-inset_x, static_cast<float>(side) * inset_y, seat_z * 0.45f},
				             {inset_x, static_cast<float>(side) * inset_y, seat_z * 0.45f}},
				radius * 0.65f, radius * 0.65f, &diagnostic);
			if (!footrest) return ChairObjectModelBuildResult::failure(diagnostic);
			add_component(&components, &parts, side < 0 ? "FrontFootrest" : "RearFootrest",
				ChairComponentCategory::Footrest, footrest);
		}
	}

	CompoundShapeSpecificationCandidate compound_candidate;
	compound_candidate.parts = std::move(parts);
	auto compound = CompoundShapeSpecificationValidator().validate(
		std::move(compound_candidate), &diagnostic);
	if (!compound) return ChairObjectModelBuildResult::failure(diagnostic);
	const std::string grammar = compound_grammar(components);
	return ChairObjectModelBuildResult::success(ChairObjectModel(
		definition, std::move(components),
		std::static_pointer_cast<const ShapeSpecification>(compound), grammar,
		placement, placement_grammar(placement)));
}
