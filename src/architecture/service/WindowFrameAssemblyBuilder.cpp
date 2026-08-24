#include "architecture/service/WindowFrameAssemblyBuilder.h"

#include "architecture/model/ArchitecturalProfileKind.h"
#include "architecture/service/ArchitecturalProfileLibrary.h"
#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/model/InstanceArrayShapeSpecification.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/FoldedProfileMeshGenerator.h"
#include "geometry/service/FoldedProfileSpecificationValidator.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "geometry/service/InstanceArrayGeometryBuilder.h"
#include "geometry/service/Profile2DFactory.h"
#include "geometry/service/SweepProfileMeshGenerator.h"
#include "geometry/service/SweepProfileSpecificationValidator.h"

#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct BuiltWindowPart
{
	std::shared_ptr<const ShapeSpecification> shape;
	GeneratedPrimitiveMesh mesh{std::make_shared<Mesh>(), {}};
};

bool includes_detail(
	GeometryDetailLevel selected_level,
	GeometryDetailLevel required_level)
{
	return geometryDetailLevelRank(selected_level) >=
	       geometryDetailLevelRank(required_level);
}

bool supports_detail(GeometryDetailLevel detail_level)
{
	const int rank = geometryDetailLevelRank(detail_level);
	return rank >= geometryDetailLevelRank(GeometryDetailLevel::Bounds) &&
	       rank <= geometryDetailLevelRank(
		       GeometryDetailLevel::FastenersAndSeals);
}

BuiltWindowPart build_extruded_rectangle(
	float width,
	float height,
	float depth,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &limits,
	std::string *diagnostic)
{
	std::shared_ptr<const Profile2D> profile =
		Profile2DFactory(limits).createRectangle(width, height, diagnostic);
	if (!profile) return {};
	ExtrudeProfileShapeSpecificationCandidate candidate;
	candidate.profile.outer_loop = profile->outerLoop().points();
	candidate.depth = depth;
	candidate.detail_level = detail_level;
	auto shape = ExtrudeProfileSpecificationValidator(limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return {};
	GeometryBuildResult built = ExtrudeProfileMeshGenerator(limits).build(*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return {};
	}
	return {shape, built.generatedMesh()};
}

BuiltWindowPart build_swept_member(
	const ArchitecturalProfileDefinition &profile,
	const std::vector<glm::vec3> &path,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &limits,
	std::string *diagnostic)
{
	SweepProfileShapeSpecificationCandidate candidate;
	candidate.profile.outer_loop = profile.crossSection().outerLoop().points();
	candidate.path_points = path;
	candidate.up_hint = glm::vec3(0.0f, 0.0f, 1.0f);
	candidate.detail_level = detail_level;
	auto shape = SweepProfileSpecificationValidator(limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return {};
	GeometryBuildResult built = SweepProfileMeshGenerator(limits).build(*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return {};
	}
	return {shape, built.generatedMesh()};
}

BuiltWindowPart build_folded_flashing(
	float width,
	const ArchitecturalProfileDefinition &profile,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &limits,
	std::string *diagnostic)
{
	constexpr float flashing_thickness = 0.0015f;
	const float half_width = width * 0.5f - flashing_thickness * 0.5f;
	FoldedProfileShapeSpecificationCandidate candidate;
	candidate.fold_path = {
		glm::vec2(-half_width, 0.0f),
		glm::vec2(half_width, 0.0f),
		glm::vec2(half_width, -profile.faceWidth())};
	candidate.thickness = flashing_thickness;
	candidate.extrusion_depth = profile.depth();
	candidate.detail_level = detail_level;
	auto shape = FoldedProfileSpecificationValidator(limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return {};
	GeometryBuildResult built = FoldedProfileMeshGenerator(limits).build(*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return {};
	}
	return {shape, built.generatedMesh()};
}

BuiltWindowPart build_anchor_array(
	float half_width,
	float half_height,
	float frame_face_width,
	float frame_depth,
	const GeometryComplexityLimits &limits,
	std::string *diagnostic)
{
	constexpr float anchor_width = 0.016f;
	constexpr float anchor_height = 0.030f;
	constexpr float anchor_depth = 0.025f;
	BuiltWindowPart source = build_extruded_rectangle(
		anchor_width,
		anchor_height,
		anchor_depth,
		GeometryDetailLevel::FastenersAndSeals,
		limits,
		diagnostic);
	if (!source.shape) return {};

	std::vector<glm::mat4> transforms;
	transforms.reserve(6u);
	const float anchor_x = half_width - frame_face_width * 0.5f;
	const float anchor_y = (half_height - frame_face_width) * 0.62f;
	for (float x : {-anchor_x, anchor_x}) {
		for (float y : {-anchor_y, 0.0f, anchor_y}) {
			transforms.push_back(glm::translate(
				glm::mat4(1.0f),
				glm::vec3(x, y, -frame_depth * 0.5f)));
		}
	}
	InstanceArraySpecification array(transforms);
	GeometryBuildResult built =
		InstanceArrayGeometryBuilder(limits).build(source.mesh, array);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return {};
	}

	std::ostringstream identity;
	identity << "WindowAnchorArray:v1:halfWidth=" << std::hexfloat << half_width
	         << ":halfHeight=" << half_height
	         << ":frameFace=" << frame_face_width
	         << ":frameDepth=" << frame_depth << ":count=" << transforms.size();
	auto array_shape = std::make_shared<const InstanceArrayShapeSpecification>(
		source.shape,
		std::move(array),
		ShapeSpecificationKey(identity.str()),
		"InstanceArray(WindowFrameAnchor count(6))",
		GeometryDetailLevel::FastenersAndSeals);
	return {array_shape, built.generatedMesh()};
}

class WindowAssemblyConstruction
{
public:
	WindowAssemblyConstruction(
		const WindowFrameSpecification &specification,
		GeometryDetailLevel detail_level,
		const GeometryComplexityLimits &complexity_limits,
		std::shared_ptr<const ArchitecturalReferenceBindingCatalog>
			reference_binding_catalog)
		: specification_(specification),
		  detail_level_(detail_level),
		  complexity_limits_(complexity_limits),
		  profile_library_(
			  complexity_limits, std::move(reference_binding_catalog))
	{
	}

	ArchitecturalAssemblyBuildResult build()
	{
		if (!validateSpecification()) {
			return ArchitecturalAssemblyBuildResult::failed(
				GeometryBuildStatus::InvalidProfile, diagnostic_);
		}
		if (!resolveProfiles()) {
			return ArchitecturalAssemblyBuildResult::failed(
				GeometryBuildStatus::InvalidProfile, diagnostic_);
		}
		if (!deriveDimensions()) {
			return ArchitecturalAssemblyBuildResult::failed(
				GeometryBuildStatus::InvalidProfile, diagnostic_);
		}
		if (!buildSelectedDetail()) {
			return ArchitecturalAssemblyBuildResult::failed(
				GeometryBuildStatus::InvalidProfile, diagnostic_);
		}

		GeometryBuildResult combined =
			GeneratedMeshComposer(complexity_limits_).compose(placements_);
		if (!combined.succeeded()) {
			return ArchitecturalAssemblyBuildResult::failed(
				combined.status(), combined.firstDiagnostic());
		}

		return ArchitecturalAssemblyBuildResult::succeeded(
			ArchitecturalAssemblyGeometry(
				std::move(parts_),
				combined.generatedMesh(),
				createInterfaces(),
				detail_level_));
	}

private:
	bool validateSpecification()
	{
		if (!supports_detail(detail_level_)) {
			diagnostic_ =
				"WindowFrame received an unsupported geometry detail level.";
			return false;
		}
		if (specification_.objectIdentifier().empty() ||
		    !std::isfinite(specification_.width()) ||
		    !std::isfinite(specification_.height()) ||
		    !std::isfinite(specification_.glazingThickness()) ||
		    specification_.width() <= 0.0f ||
		    specification_.height() <= 0.0f ||
		    specification_.glazingThickness() <= 0.0f ||
		    specification_.mullionCount() < 0 ||
		    specification_.mullionCount() > 64) {
			diagnostic_ =
				"WindowFrame requires an identifier, finite positive dimensions, and 0-64 mullions.";
			return false;
		}
		return true;
	}

	bool resolveProfiles()
	{
		frame_profile_ = profile_library_.find(
			ArchitecturalProfileKind::AluminiumWindowFrame, &diagnostic_);
		mullion_profile_ = profile_library_.find(
			ArchitecturalProfileKind::AluminiumWindowMullion, &diagnostic_);
		gasket_profile_ = profile_library_.find(
			ArchitecturalProfileKind::WindowGasket, &diagnostic_);
		flashing_profile_ = profile_library_.find(
			ArchitecturalProfileKind::SheetMetalFlashing, &diagnostic_);
		return frame_profile_ && mullion_profile_ && gasket_profile_ &&
		       flashing_profile_;
	}

	bool deriveDimensions()
	{
		if (specification_.width() <= frame_profile_->faceWidth() * 2.0f ||
		    specification_.height() <= frame_profile_->faceWidth() * 2.0f ||
		    specification_.glazingThickness() >= frame_profile_->depth()) {
			diagnostic_ =
				"WindowFrame dimensions are too small for the selected frame and glazing profiles.";
			return false;
		}
		half_width_ = specification_.width() * 0.5f;
		half_height_ = specification_.height() * 0.5f;
		inner_half_width_ = half_width_ - frame_profile_->faceWidth();
		inner_half_height_ = half_height_ - frame_profile_->faceWidth();
		glazing_half_width_ = inner_half_width_;
		glazing_half_height_ = inner_half_height_;
		if (includes_detail(
				detail_level_, GeometryDetailLevel::ConstructionDetail)) {
			glazing_half_width_ -= gasket_profile_->faceWidth();
			glazing_half_height_ -= gasket_profile_->faceWidth();
		}
		if (glazing_half_width_ <= 0.0f || glazing_half_height_ <= 0.0f) {
			diagnostic_ =
				"WindowFrame has no space for its selected glazing detail.";
			return false;
		}
		return true;
	}

	bool buildSelectedDetail()
	{
		if (detail_level_ == GeometryDetailLevel::Bounds) {
			return buildBounds();
		}
		if (includes_detail(detail_level_, GeometryDetailLevel::Component)) {
			if (!buildDetailedFrame()) return false;
		} else if (!buildSimplifiedFrame()) {
			return false;
		}
		if (includes_detail(detail_level_, GeometryDetailLevel::Assembly) &&
		    !buildMullions()) {
			return false;
		}
		if (!buildGlazing()) return false;
		if (includes_detail(
				detail_level_, GeometryDetailLevel::ConstructionDetail) &&
		    (!buildGaskets() || !buildFlashings())) {
			return false;
		}
		if (includes_detail(
				detail_level_, GeometryDetailLevel::FastenersAndSeals) &&
		    !buildAnchors()) {
			return false;
		}
		return true;
	}

	bool appendPart(
		std::string identifier,
		std::string role,
		std::string material,
		BuiltWindowPart built,
		const glm::mat4 &transform,
		GeometryDetailRange detail_range)
	{
		if (!built.shape) return false;
		placements_.emplace_back(identifier, built.mesh, transform);
		parts_.emplace_back(
			std::move(identifier),
			std::move(role),
			std::move(material),
			std::move(built.shape),
			std::move(built.mesh),
			transform,
			detail_range);
		return true;
	}

	bool buildBounds()
	{
		const float minimum_depth_position =
			-frame_profile_->depth() * 0.5f - flashing_profile_->depth() * 0.5f;
		const float maximum_depth_position = frame_profile_->depth() * 0.5f;
		return appendPart(
			"window_bounds",
			"WindowBounds",
			"spatialdebugbounds",
			build_extruded_rectangle(
				specification_.width(),
				specification_.height(),
				maximum_depth_position - minimum_depth_position,
				detail_level_,
				complexity_limits_,
				&diagnostic_),
			glm::translate(
				glm::mat4(1.0f),
				glm::vec3(0.0f, 0.0f, minimum_depth_position)),
			GeometryDetailRange::exact(GeometryDetailLevel::Bounds));
	}

	bool buildSimplifiedFrame()
	{
		const GeometryDetailRange range(
			GeometryDetailLevel::CoarseShape,
			GeometryDetailLevel::Assembly);
		const float face_width = frame_profile_->faceWidth();
		const float face_half = face_width * 0.5f;
		const float depth = frame_profile_->depth();
		const glm::mat4 depth_transform = glm::translate(
			glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -depth * 0.5f));
		for (const auto &member : std::vector<std::pair<std::string, float>>{
			     {"frame_sill", -half_height_ + face_half},
			     {"frame_head", half_height_ - face_half}}) {
			if (!appendPart(
					member.first,
					member.first == "frame_sill" ? "FrameSill" : "FrameHead",
					"silverbluepolishedmetal",
					build_extruded_rectangle(
						specification_.width() - face_width * 2.0f,
						face_width,
						depth,
						detail_level_,
						complexity_limits_,
						&diagnostic_),
					glm::translate(
						depth_transform, glm::vec3(0.0f, member.second, 0.0f)),
					range)) {
				return false;
			}
		}
		for (const auto &member : std::vector<std::pair<std::string, float>>{
			     {"frame_left_jamb", -half_width_ + face_half},
			     {"frame_right_jamb", half_width_ - face_half}}) {
			if (!appendPart(
					member.first,
					"FrameJamb",
					"silverbluepolishedmetal",
					build_extruded_rectangle(
						face_width,
						specification_.height(),
						depth,
						detail_level_,
						complexity_limits_,
						&diagnostic_),
					glm::translate(
						depth_transform, glm::vec3(member.second, 0.0f, 0.0f)),
					range)) {
				return false;
			}
		}
		return true;
	}

	bool appendSweptMember(
		const std::string &identifier,
		const std::string &role,
		const ArchitecturalProfileDefinition &profile,
		const std::vector<glm::vec3> &path,
		GeometryDetailRange range)
	{
		return appendPart(
			identifier,
			role,
			"silverbluepolishedmetal",
			build_swept_member(
				profile,
				path,
				detail_level_,
				complexity_limits_,
				&diagnostic_),
			glm::mat4(1.0f),
			range);
	}

	bool buildDetailedFrame()
	{
		const GeometryDetailRange range(
			GeometryDetailLevel::Component,
			GeometryDetailLevel::FastenersAndSeals);
		const float face_half = frame_profile_->faceWidth() * 0.5f;
		const float horizontal_half = half_width_ - face_half;
		const float vertical_half = half_height_ - face_half;
		return appendSweptMember(
			       "frame_sill",
			       "FrameSill",
			       *frame_profile_,
			       {{-horizontal_half, -vertical_half, 0.0f},
			        {horizontal_half, -vertical_half, 0.0f}},
			       range) &&
		       appendSweptMember(
			       "frame_head",
			       "FrameHead",
			       *frame_profile_,
			       {{-horizontal_half, vertical_half, 0.0f},
			        {horizontal_half, vertical_half, 0.0f}},
			       range) &&
		       appendSweptMember(
			       "frame_left_jamb",
			       "FrameJamb",
			       *frame_profile_,
			       {{-horizontal_half, -vertical_half, 0.0f},
			        {-horizontal_half, vertical_half, 0.0f}},
			       range) &&
		       appendSweptMember(
			       "frame_right_jamb",
			       "FrameJamb",
			       *frame_profile_,
			       {{horizontal_half, -vertical_half, 0.0f},
			        {horizontal_half, vertical_half, 0.0f}},
			       range);
	}

	bool buildMullions()
	{
		const GeometryDetailRange range(
			GeometryDetailLevel::Assembly,
			GeometryDetailLevel::FastenersAndSeals);
		for (int index = 0; index < specification_.mullionCount(); ++index) {
			const float fraction = static_cast<float>(index + 1) /
			                       static_cast<float>(specification_.mullionCount() + 1);
			const float x =
				-inner_half_width_ + 2.0f * inner_half_width_ * fraction;
			BuiltWindowPart built;
			glm::mat4 transform(1.0f);
			if (includes_detail(detail_level_, GeometryDetailLevel::Component)) {
				built = build_swept_member(
					*mullion_profile_,
					{{x, -inner_half_height_, 0.0f},
					 {x, inner_half_height_, 0.0f}},
					detail_level_,
					complexity_limits_,
					&diagnostic_);
			} else {
				built = build_extruded_rectangle(
					mullion_profile_->faceWidth(),
					inner_half_height_ * 2.0f,
					mullion_profile_->depth(),
					detail_level_,
					complexity_limits_,
					&diagnostic_);
				transform = glm::translate(
					glm::mat4(1.0f),
					glm::vec3(
						x,
						0.0f,
						-mullion_profile_->depth() * 0.5f));
			}
			if (!appendPart(
					"mullion_" + std::to_string(index + 1),
					"Mullion",
					"silverbluepolishedmetal",
					std::move(built),
					transform,
					range)) {
				return false;
			}
		}
		return true;
	}

	bool buildGlazing()
	{
		return appendPart(
			"glazing_unit",
			"InsulatedGlazingUnit",
			"smokytransparentglass",
			build_extruded_rectangle(
				glazing_half_width_ * 2.0f,
				glazing_half_height_ * 2.0f,
				specification_.glazingThickness(),
				detail_level_,
				complexity_limits_,
				&diagnostic_),
			glm::translate(
				glm::mat4(1.0f),
				glm::vec3(
					0.0f,
					0.0f,
					-specification_.glazingThickness() * 0.5f)),
			GeometryDetailRange(
				GeometryDetailLevel::CoarseShape,
				GeometryDetailLevel::FastenersAndSeals));
	}

	bool buildGaskets()
	{
		const GeometryDetailRange range(
			GeometryDetailLevel::ConstructionDetail,
			GeometryDetailLevel::FastenersAndSeals);
		const std::vector<std::pair<std::string, std::vector<glm::vec3>>> paths = {
			{"gasket_bottom",
			 {{-glazing_half_width_, -glazing_half_height_, 0.0f},
			  {glazing_half_width_, -glazing_half_height_, 0.0f}}},
			{"gasket_top",
			 {{-glazing_half_width_, glazing_half_height_, 0.0f},
			  {glazing_half_width_, glazing_half_height_, 0.0f}}},
			{"gasket_left",
			 {{-glazing_half_width_, -glazing_half_height_, 0.0f},
			  {-glazing_half_width_, glazing_half_height_, 0.0f}}},
			{"gasket_right",
			 {{glazing_half_width_, -glazing_half_height_, 0.0f},
			  {glazing_half_width_, glazing_half_height_, 0.0f}}}};
		for (const auto &path : paths) {
			if (!appendPart(
					path.first,
					"GlazingGasket",
					"charcoalroughmetal",
					build_swept_member(
						*gasket_profile_,
						path.second,
						detail_level_,
						complexity_limits_,
						&diagnostic_),
					glm::mat4(1.0f),
					range)) {
				return false;
			}
		}
		return true;
	}

	bool buildFlashings()
	{
		const GeometryDetailRange range(
			GeometryDetailLevel::ConstructionDetail,
			GeometryDetailLevel::FastenersAndSeals);
		BuiltWindowPart sill = build_folded_flashing(
			specification_.width(),
			*flashing_profile_,
			detail_level_,
			complexity_limits_,
			&diagnostic_);
		if (!appendPart(
				"sill_flashing",
				"SillFlashing",
				"silverbluemattesheetmetal",
				std::move(sill),
				glm::translate(
					glm::mat4(1.0f),
					glm::vec3(
						0.0f,
						-half_height_ + flashing_profile_->faceWidth(),
						-frame_profile_->depth() * 0.5f)),
				range)) {
			return false;
		}
		BuiltWindowPart head = build_folded_flashing(
			specification_.width(),
			*flashing_profile_,
			detail_level_,
			complexity_limits_,
			&diagnostic_);
		const glm::mat4 head_transform =
			glm::translate(
				glm::mat4(1.0f),
				glm::vec3(
					0.0f,
					half_height_ - flashing_profile_->faceWidth(),
					-frame_profile_->depth() * 0.5f)) *
			glm::rotate(
				glm::mat4(1.0f),
				3.14159265358979323846f,
				glm::vec3(0.0f, 0.0f, 1.0f));
		return appendPart(
			"head_flashing",
			"HeadFlashing",
			"silverbluemattesheetmetal",
			std::move(head),
			head_transform,
			range);
	}

	bool buildAnchors()
	{
		return appendPart(
			"frame_anchor_array",
			"FrameAnchorArray",
			"darkslatesmoothmetal",
			build_anchor_array(
				half_width_,
				half_height_,
				frame_profile_->faceWidth(),
				frame_profile_->depth(),
				complexity_limits_,
				&diagnostic_),
			glm::mat4(1.0f),
			GeometryDetailRange::exact(
				GeometryDetailLevel::FastenersAndSeals));
	}

	std::vector<SpatialInterface> createInterfaces() const
	{
		const SpatialObjectId owner(specification_.objectIdentifier());
		std::vector<SpatialInterface> interfaces;
		interfaces.emplace_back(
			SpatialInterfaceId("frame_perimeter"),
			owner,
			SpatialInterfaceType::Mate,
			SpatialInterfaceFrame(
				glm::vec3(0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f),
				glm::vec3(1.0f, 0.0f, 0.0f)),
			SpatialInterfaceRegion::planeRectangle(
				specification_.width(), specification_.height()),
			InterfaceCompatibilityProfile(
				InterfaceShape::Rectangular,
				InterfaceGender::Male,
				std::nullopt,
				specification_.width(),
				specification_.height(),
				"HostedWindowOpening"),
			SpatialClearanceRequirement(
				specification_.perimeterClearance(),
				specification_.perimeterClearance(),
				specification_.perimeterClearance()));
		interfaces.emplace_back(
			SpatialInterfaceId("glazing_seat"),
			owner,
			SpatialInterfaceType::Seat,
			SpatialInterfaceFrame(
				glm::vec3(0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f),
				glm::vec3(1.0f, 0.0f, 0.0f)),
			SpatialInterfaceRegion::planeRectangle(
				glazing_half_width_ * 2.0f,
				glazing_half_height_ * 2.0f),
			InterfaceCompatibilityProfile(
				InterfaceShape::Rectangular,
				InterfaceGender::Neutral,
				std::nullopt,
				glazing_half_width_ * 2.0f,
				glazing_half_height_ * 2.0f,
				"GlazingSeat"),
			SpatialClearanceRequirement(0.0f, 0.0f, 0.001f));
		return interfaces;
	}

	const WindowFrameSpecification &specification_;
	GeometryDetailLevel detail_level_ = GeometryDetailLevel::FastenersAndSeals;
	GeometryComplexityLimits complexity_limits_;
	ArchitecturalProfileLibrary profile_library_;
	std::shared_ptr<const ArchitecturalProfileDefinition> frame_profile_;
	std::shared_ptr<const ArchitecturalProfileDefinition> mullion_profile_;
	std::shared_ptr<const ArchitecturalProfileDefinition> gasket_profile_;
	std::shared_ptr<const ArchitecturalProfileDefinition> flashing_profile_;
	float half_width_ = 0.0f;
	float half_height_ = 0.0f;
	float inner_half_width_ = 0.0f;
	float inner_half_height_ = 0.0f;
	float glazing_half_width_ = 0.0f;
	float glazing_half_height_ = 0.0f;
	std::string diagnostic_;
	std::vector<ArchitecturalAssemblyPart> parts_;
	std::vector<GeneratedMeshPlacement> placements_;
};

}

ArchitecturalAssemblyBuildResult WindowFrameAssemblyBuilder::build(
	const WindowFrameSpecification &specification,
	GeometryDetailLevel detail_level) const
{
	return WindowAssemblyConstruction(
		specification,
		detail_level,
		complexity_limits_,
		reference_binding_catalog_)
		.build();
}
