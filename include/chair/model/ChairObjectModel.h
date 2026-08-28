#pragma once

#include "geometry/model/ShapeSpecification.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class ChairUseContext
{
	Dining,
	Kitchen,
	Study,
	Living,
	Patio
};

enum class ChairSupportKind
{
	FourLeg,
	Cantilever,
	Sled,
	Pedestal,
	SwivelCaster
};

enum class ChairComponentCategory
{
	Seat,
	Backrest,
	Armrest,
	Cushion,
	Footrest,
	Support
};

class ChairDimensionSpecification
{
public:
	ChairDimensionSpecification(
		float width,
		float depth,
		float seat_height,
		float overall_height)
		: width_(width), depth_(depth), seat_height_(seat_height),
		  overall_height_(overall_height)
	{
	}

	float width() const { return width_; }
	float depth() const { return depth_; }
	float seatHeight() const { return seat_height_; }
	float overallHeight() const { return overall_height_; }

private:
	float width_ = 0.5f;
	float depth_ = 0.5f;
	float seat_height_ = 0.45f;
	float overall_height_ = 0.85f;
};

class ChairErgonomicSpecification
{
public:
	ChairErgonomicSpecification(
		float seat_slope_degrees,
		float back_recline_degrees,
		float lumbar_height)
		: seat_slope_degrees_(seat_slope_degrees),
		  back_recline_degrees_(back_recline_degrees),
		  lumbar_height_(lumbar_height)
	{
	}

	float seatSlopeDegrees() const { return seat_slope_degrees_; }
	float backReclineDegrees() const { return back_recline_degrees_; }
	float lumbarHeight() const { return lumbar_height_; }

private:
	float seat_slope_degrees_ = 0.0f;
	float back_recline_degrees_ = 8.0f;
	float lumbar_height_ = 0.18f;
};

class ChairGeometryParameterSet
{
public:
	ChairGeometryParameterSet(
		ChairSupportKind support_kind,
		float member_radius,
		float shell_camber,
		float shell_thickness,
		bool has_backrest,
		bool has_armrests,
		bool has_cushion,
		bool has_footrest)
		: support_kind_(support_kind), member_radius_(member_radius),
		  shell_camber_(shell_camber), shell_thickness_(shell_thickness),
		  has_backrest_(has_backrest), has_armrests_(has_armrests),
		  has_cushion_(has_cushion), has_footrest_(has_footrest)
	{
	}

	ChairSupportKind supportKind() const { return support_kind_; }
	float memberRadius() const { return member_radius_; }
	float shellCamber() const { return shell_camber_; }
	float shellThickness() const { return shell_thickness_; }
	bool hasBackrest() const { return has_backrest_; }
	bool hasArmrests() const { return has_armrests_; }
	bool hasCushion() const { return has_cushion_; }
	bool hasFootrest() const { return has_footrest_; }

private:
	ChairSupportKind support_kind_ = ChairSupportKind::FourLeg;
	float member_radius_ = 0.025f;
	float shell_camber_ = 0.025f;
	float shell_thickness_ = 0.025f;
	bool has_backrest_ = true;
	bool has_armrests_ = false;
	bool has_cushion_ = false;
	bool has_footrest_ = false;
};

class ChairMaterialVariantSet
{
public:
	explicit ChairMaterialVariantSet(std::vector<std::string> material_identifiers)
		: material_identifiers_(std::move(material_identifiers))
	{
	}

	const std::vector<std::string> &materialIdentifiers() const
	{
		return material_identifiers_;
	}

private:
	std::vector<std::string> material_identifiers_;
};

class ChairContextApplicabilityRelationship
{
public:
	explicit ChairContextApplicabilityRelationship(
		std::vector<ChairUseContext> contexts)
		: contexts_(std::move(contexts))
	{
	}

	const std::vector<ChairUseContext> &contexts() const { return contexts_; }
	bool appliesTo(ChairUseContext context) const;

private:
	std::vector<ChairUseContext> contexts_;
};

class ChairDesignDefinition
{
public:
	ChairDesignDefinition(
		std::string identifier,
		std::string display_name,
		ChairContextApplicabilityRelationship applicability,
		ChairDimensionSpecification dimensions,
		ChairErgonomicSpecification ergonomics,
		ChairGeometryParameterSet geometry,
		ChairMaterialVariantSet materials)
		: identifier_(std::move(identifier)), display_name_(std::move(display_name)),
		  applicability_(std::move(applicability)), dimensions_(dimensions),
		  ergonomics_(ergonomics), geometry_(geometry), materials_(std::move(materials))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &displayName() const { return display_name_; }
	const ChairContextApplicabilityRelationship &applicability() const { return applicability_; }
	const ChairDimensionSpecification &dimensions() const { return dimensions_; }
	const ChairErgonomicSpecification &ergonomics() const { return ergonomics_; }
	const ChairGeometryParameterSet &geometry() const { return geometry_; }
	const ChairMaterialVariantSet &materials() const { return materials_; }

private:
	std::string identifier_;
	std::string display_name_;
	ChairContextApplicabilityRelationship applicability_{{}};
	ChairDimensionSpecification dimensions_{0.5f, 0.5f, 0.45f, 0.85f};
	ChairErgonomicSpecification ergonomics_{0.0f, 8.0f, 0.18f};
	ChairGeometryParameterSet geometry_{
		ChairSupportKind::FourLeg, 0.025f, 0.025f, 0.025f, true, false, false, false};
	ChairMaterialVariantSet materials_{{}};
};

class ChairComponent
{
public:
	ChairComponent(
		std::string purpose,
		ChairComponentCategory category,
		std::shared_ptr<const ShapeSpecification> shape)
		: purpose_(std::move(purpose)), category_(category), shape_(std::move(shape))
	{
	}

	const std::string &purpose() const { return purpose_; }
	ChairComponentCategory category() const { return category_; }
	const std::shared_ptr<const ShapeSpecification> &shape() const { return shape_; }

private:
	std::string purpose_;
	ChairComponentCategory category_ = ChairComponentCategory::Support;
	std::shared_ptr<const ShapeSpecification> shape_;
};

class ChairReferenceViewSet
{
public:
	ChairReferenceViewSet(std::string front, std::string side, std::string top)
		: front_(std::move(front)), side_(std::move(side)), top_(std::move(top))
	{
	}
	const std::string &front() const { return front_; }
	const std::string &side() const { return side_; }
	const std::string &top() const { return top_; }
private:
	std::string front_;
	std::string side_;
	std::string top_;
};

class ChairDesignAcceptanceRecord
{
public:
	ChairDesignAcceptanceRecord(double front_iou, double side_iou, double top_iou)
		: front_iou_(front_iou), side_iou_(side_iou), top_iou_(top_iou)
	{
	}
	double frontIoU() const { return front_iou_; }
	double sideIoU() const { return side_iou_; }
	double topIoU() const { return top_iou_; }
	bool passes(double threshold) const;
private:
	double front_iou_ = 0.0;
	double side_iou_ = 0.0;
	double top_iou_ = 0.0;
};

class ChairOrientation
{
public:
	explicit ChairOrientation(float yaw_degrees = 0.0f)
		: yaw_degrees_(yaw_degrees)
	{
	}

	float yawDegrees() const { return yaw_degrees_; }
	float localToWorldPitchDegrees() const { return -90.0f; }
	glm::vec3 localUpDirection() const;
	glm::vec3 localForwardDirection() const;
	glm::vec3 worldUpDirection() const;
	glm::vec3 worldForwardDirection() const;
	glm::vec3 worldRightDirection() const;
	glm::mat4 localToWorldRotation() const;
	bool isFinite() const;

private:
	float yaw_degrees_ = 0.0f;
};

class ChairPlacement
{
public:
	ChairPlacement(
		glm::vec3 ground_contact_position,
		ChairOrientation orientation,
		float local_ground_height = 0.0f)
		: ground_contact_position_(ground_contact_position),
		  orientation_(orientation),
		  local_ground_height_(local_ground_height)
	{
	}

	static ChairPlacement atWorldOrigin();

	const glm::vec3 &groundContactPosition() const { return ground_contact_position_; }
	const ChairOrientation &orientation() const { return orientation_; }
	float localGroundHeight() const { return local_ground_height_; }
	glm::mat4 worldTransform() const;
	glm::vec3 worldPositionOfLocalPoint(const glm::vec3 &local_point) const;
	bool isValid() const;

private:
	glm::vec3 ground_contact_position_{0.0f};
	ChairOrientation orientation_{};
	float local_ground_height_ = 0.0f;
};

class ChairObjectModel
{
public:
	ChairObjectModel(
		ChairDesignDefinition definition,
		std::vector<ChairComponent> components,
		std::shared_ptr<const ShapeSpecification> assembly_shape,
		std::string grammar_text,
		ChairPlacement placement,
		std::string placement_grammar_text)
		: definition_(std::move(definition)), components_(std::move(components)),
		  assembly_shape_(std::move(assembly_shape)), grammar_text_(std::move(grammar_text)),
		  placement_(placement), placement_grammar_text_(std::move(placement_grammar_text))
	{
	}

	const ChairDesignDefinition &definition() const { return definition_; }
	const std::vector<ChairComponent> &components() const { return components_; }
	const std::shared_ptr<const ShapeSpecification> &assemblyShape() const { return assembly_shape_; }
	const std::string &grammarText() const { return grammar_text_; }
	const ChairPlacement &placement() const { return placement_; }
	const std::string &placementGrammarText() const { return placement_grammar_text_; }
	glm::mat4 worldTransform() const { return placement_.worldTransform(); }

private:
	ChairDesignDefinition definition_;
	std::vector<ChairComponent> components_;
	std::shared_ptr<const ShapeSpecification> assembly_shape_;
	std::string grammar_text_;
	ChairPlacement placement_ = ChairPlacement::atWorldOrigin();
	std::string placement_grammar_text_;
};

class ChairObjectModelBuildResult
{
public:
	static ChairObjectModelBuildResult success(ChairObjectModel model);
	static ChairObjectModelBuildResult failure(std::string diagnostic);
	bool succeeded() const { return model_.has_value(); }
	const std::optional<ChairObjectModel> &model() const { return model_; }
	const std::string &diagnostic() const { return diagnostic_; }
private:
	ChairObjectModelBuildResult(
		std::optional<ChairObjectModel> model,
		std::string diagnostic)
		: model_(std::move(model)), diagnostic_(std::move(diagnostic))
	{
	}
	std::optional<ChairObjectModel> model_;
	std::string diagnostic_;
};

const char *chairUseContextName(ChairUseContext context);
const char *chairSupportKindName(ChairSupportKind support_kind);
