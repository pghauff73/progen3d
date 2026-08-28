#include "grammar/service/ShapeSpecificationEvaluator.h"

#include "geometry/model/AxialProfileShapeSpecification.h"
#include "geometry/model/BotanicalBladeProfile.h"
#include "geometry/model/BranchJunctionShapeSpecification.h"
#include "geometry/model/CylinderShapeSpecification.h"
#include "geometry/model/CompoundShapeSpecification.h"
#include "geometry/model/CurvedPanelShapeSpecification.h"
#include "geometry/model/CurveNetworkSurfaceShapeSpecification.h"
#include "geometry/model/EdgeFlangeShapeSpecification.h"
#include "geometry/model/EmbossedBeadShapeSpecification.h"
#include "geometry/model/ExtrudeProfileShapeSpecification.h"
#include "geometry/model/FoldedProfileShapeSpecification.h"
#include "geometry/model/FormedPanelShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/InstanceArrayShapeSpecification.h"
#include "geometry/model/InstanceArraySpecification.h"
#include "geometry/model/HostedOpeningShapeSpecification.h"
#include "geometry/model/LoftShapeSpecification.h"
#include "geometry/model/MirrorShapeSpecification.h"
#include "geometry/model/RevolveShapeSpecification.h"
#include "geometry/model/ShellLoftShapeSpecification.h"
#include "geometry/model/ShellOffsetShapeSpecification.h"
#include "geometry/model/SurfaceLoftShapeSpecification.h"
#include "geometry/model/SphereShapeSpecification.h"
#include "geometry/model/SweepDiskShapeSpecification.h"
#include "geometry/model/SweepProfileShapeSpecification.h"
#include "geometry/model/TaperedSweepShapeSpecification.h"
#include "geometry/model/VariableSectionSweepShapeSpecification.h"
#include "geometry/service/AxialProfileSpecificationValidator.h"
#include "geometry/service/BotanicalBladeSpecificationValidator.h"
#include "geometry/service/BranchJunctionSpecificationValidator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/GeneratedMeshReferenceSpecificationValidator.h"
#include "geometry/service/CurvedPanelSpecificationValidator.h"
#include "geometry/service/CurveNetworkSurfaceSpecificationValidator.h"
#include "geometry/service/CompoundShapeSpecificationValidator.h"
#include "geometry/service/EdgeFlangeSpecificationValidator.h"
#include "geometry/service/EmbossedBeadSpecificationValidator.h"
#include "geometry/service/FoldedProfileSpecificationValidator.h"
#include "geometry/service/FormedPanelSpecificationValidator.h"
#include "geometry/service/HostedOpeningSpecificationValidator.h"
#include "geometry/service/LoftSpecificationValidator.h"
#include "geometry/service/Profile2DFactory.h"
#include "geometry/service/RevolveSpecificationValidator.h"
#include "geometry/service/ShapeAliasExpansionService.h"
#include "geometry/service/ShapeSpecificationValidator.h"
#include "geometry/service/SweepDiskSpecificationValidator.h"
#include "geometry/service/SweepProfileSpecificationValidator.h"
#include "geometry/service/TaperedSweepSpecificationValidator.h"
#include "geometry/service/ThinWallProfileFactory.h"
#include "geometry/service/VariableSectionSweepSpecificationValidator.h"
#include "grammar/model/AxialProfileDescriptorSyntax.h"
#include "grammar/model/CompoundShapeDescriptorSyntax.h"
#include "grammar/model/ExtrudeProfileDescriptorSyntax.h"
#include "grammar/model/InstanceArrayDescriptorSyntax.h"
#include "grammar/model/NestedSourceShapeDescriptorSyntax.h"
#include "vegetation/model/PlantShapeSpecification.h"
#include "vegetation/model/PlantOrganArraySpecification.h"
#include "vegetation/model/PlantTopologyGenerationMethod.h"
#include "vegetation/model/ScatterRegionShapeSpecification.h"
#include "vegetation/model/VegetationSurfaceGeometryKind.h"
#include "vegetation/model/VineShapeSpecification.h"
#include "vegetation/service/PlantShapeSpecificationValidator.h"
#include "vegetation/service/PlantSpeciesCatalog.h"
#include "vegetation/service/ScatterRegionShapeSpecificationValidator.h"
#include "vegetation/service/VineShapeSpecificationValidator.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <utility>

namespace {

std::string lowercase_copy(const std::string &value)
{
	std::string lowered = value;
	std::transform(
		lowered.begin(),
		lowered.end(),
		lowered.begin(),
		[](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
	return lowered;
}

bool evaluate_number(const GeometryExpression &expression,
	                 const std::string &purpose,
	                 const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator,
	                 float *value)
{
	return evaluator(expression, purpose, value) && std::isfinite(*value);
}

bool evaluate_integer(const GeometryExpression &expression,
	                  const std::string &purpose,
	                  const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator,
	                  int *value,
	                  std::string *diagnostic)
{
	float evaluated = 0.0f;
	if (!evaluate_number(expression, purpose, evaluator, &evaluated)) {
		return false;
	}
	const float rounded = std::round(evaluated);
	if (std::fabs(evaluated - rounded) > 1.0e-5f ||
	    rounded < static_cast<float>(std::numeric_limits<int>::lowest()) ||
	    rounded > static_cast<float>(std::numeric_limits<int>::max())) {
		if (diagnostic != nullptr) {
			*diagnostic = purpose + " must evaluate to an integer.";
		}
		return false;
	}
	*value = static_cast<int>(rounded);
	return true;
}

AxisAlignedBounds evaluated_bounds(
	const std::vector<GeometryExpression> &arguments,
	std::size_t first_coordinate,
	const std::string &purpose,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator,
	bool *succeeded)
{
	AxisAlignedBounds bounds;
	float values[6] = {};
	for (std::size_t index = 0u; index < 6u; ++index) {
		if (!evaluate_number(
				arguments[first_coordinate + index], purpose, evaluator,
				&values[index])) {
			*succeeded = false;
			return bounds;
		}
	}
	bounds.min = glm::vec3(values[0], values[1], values[2]);
	bounds.max = glm::vec3(values[3], values[4], values[5]);
	bounds.center = (bounds.min + bounds.max) * 0.5f;
	bounds.half_extents = (bounds.max - bounds.min) * 0.5f;
	bounds.valid = true;
	*succeeded = true;
	return bounds;
}

bool parse_side(const GeometryExpression &expression,
	            ClipRetainedSide *side,
	            std::string *diagnostic)
{
	if (expression.sourceText() == "positive") {
		*side = ClipRetainedSide::Positive;
		return true;
	}
	if (expression.sourceText() == "negative") {
		*side = ClipRetainedSide::Negative;
		return true;
	}
	if (diagnostic != nullptr) {
		*diagnostic = "Clip side must be 'positive' or 'negative'.";
	}
	return false;
}

bool parse_extrude_cap_policy(
	const GeometryExpression &expression,
	ExtrudeProfileCapPolicy *cap_policy,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "all") {
		*cap_policy = ExtrudeProfileCapPolicy::createAll();
		return true;
	}
	if (name == "none") {
		*cap_policy = ExtrudeProfileCapPolicy::createNone();
		return true;
	}
	if (name == "front") {
		*cap_policy = ExtrudeProfileCapPolicy::createFront();
		return true;
	}
	if (name == "back") {
		*cap_policy = ExtrudeProfileCapPolicy::createBack();
		return true;
	}
	if (diagnostic != nullptr) {
		*diagnostic = "Cap policy must be all, none, front, or back.";
	}
	return false;
}

bool parse_botanical_blade_profile(
	const GeometryExpression &expression,
	BotanicalBladeProfile *profile,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "linear") *profile = BotanicalBladeProfile::Linear;
	else if (name == "lanceolate") *profile = BotanicalBladeProfile::Lanceolate;
	else if (name == "elliptic") *profile = BotanicalBladeProfile::Elliptic;
	else if (name == "ovate") *profile = BotanicalBladeProfile::Ovate;
	else if (name == "obovate") *profile = BotanicalBladeProfile::Obovate;
	else if (name == "cordate") *profile = BotanicalBladeProfile::Cordate;
	else if (name == "palmate") *profile = BotanicalBladeProfile::Palmate;
	else if (name == "needle") *profile = BotanicalBladeProfile::Needle;
	else {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Botanical blade profile must be Linear, Lanceolate, Elliptic, Ovate, Obovate, Cordate, Palmate, or Needle.";
		}
		return false;
	}
	return true;
}

bool parse_plant_architecture(
	const GeometryExpression &expression,
	PlantArchitecture *architecture,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "tree") *architecture = PlantArchitecture::Tree;
	else if (name == "shrub") *architecture = PlantArchitecture::Shrub;
	else if (name == "herb") *architecture = PlantArchitecture::Herb;
	else if (name == "grass") *architecture = PlantArchitecture::Grass;
	else if (name == "vine") *architecture = PlantArchitecture::Vine;
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant architecture must be Tree, Shrub, Herb, Grass, or Vine.";
		}
		return false;
	}
	return true;
}

bool parse_plant_development_state(
	const GeometryExpression &expression,
	PlantDevelopmentState *state,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "seed") *state = PlantDevelopmentState::Seed;
	else if (name == "bud") *state = PlantDevelopmentState::Bud;
	else if (name == "shoot") *state = PlantDevelopmentState::Shoot;
	else if (name == "juvenile") *state = PlantDevelopmentState::Juvenile;
	else if (name == "mature") *state = PlantDevelopmentState::Mature;
	else if (name == "flowering") *state = PlantDevelopmentState::Flowering;
	else if (name == "fruiting") *state = PlantDevelopmentState::Fruiting;
	else if (name == "senescent") *state = PlantDevelopmentState::Senescent;
	else if (name == "dormant") *state = PlantDevelopmentState::Dormant;
	else if (name == "dead") *state = PlantDevelopmentState::Dead;
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant state must be Seed, Bud, Shoot, Juvenile, Mature, Flowering, Fruiting, Senescent, Dormant, or Dead.";
		}
		return false;
	}
	return true;
}

bool parse_plant_topology_generation_method(
	const GeometryExpression &expression,
	PlantTopologyGenerationMethod *method,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "rulebranching") {
		*method = PlantTopologyGenerationMethod::RuleBranching;
	}
	else if (name == "spacecolonization") {
		*method = PlantTopologyGenerationMethod::SpaceColonization;
	}
	else if (name == "lsystem") {
		*method = PlantTopologyGenerationMethod::LSystem;
	}
	else {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Plant generation must be RuleBranching, SpaceColonization, or LSystem.";
		}
		return false;
	}
	return true;
}

bool parse_l_system_symbol(
	const GeometryExpression &expression,
	LSystemSymbol *symbol,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "f" || name == "forward") {
		*symbol = LSystemSymbol::Forward;
	}
	else if (name == "plus" || name == "turnpositive") {
		*symbol = LSystemSymbol::TurnPositive;
	}
	else if (name == "minus" || name == "turnnegative") {
		*symbol = LSystemSymbol::TurnNegative;
	}
	else if (name == "push" || name == "pushstate") {
		*symbol = LSystemSymbol::PushState;
	}
	else if (name == "pop" || name == "popstate") {
		*symbol = LSystemSymbol::PopState;
	}
	else {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Plant L-system symbols must be F, Plus, Minus, Push, or Pop.";
		}
		return false;
	}
	return true;
}

bool parse_crown_volume_kind(
	const GeometryExpression &expression,
	CrownVolumeKind *kind,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "sphere") *kind = CrownVolumeKind::Sphere;
	else if (name == "ellipsoid") *kind = CrownVolumeKind::Ellipsoid;
	else if (name == "cone") *kind = CrownVolumeKind::Cone;
	else if (name == "inversecone") *kind = CrownVolumeKind::InverseCone;
	else if (name == "cylinder") *kind = CrownVolumeKind::Cylinder;
	else if (name == "dome") *kind = CrownVolumeKind::Dome;
	else if (name == "lobed") *kind = CrownVolumeKind::Lobed;
	else {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Plant crown must be Sphere, Ellipsoid, Cone, InverseCone, Cylinder, Dome, or Lobed; use crownCustom for sampled volumes.";
		}
		return false;
	}
	return true;
}

bool parse_growth_curve(
	const GeometryExpression &expression,
	GrowthCurve *curve,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "linear") *curve = GrowthCurve::Linear;
	else if (name == "smoothstep") *curve = GrowthCurve::SmoothStep;
	else if (name == "easein") *curve = GrowthCurve::EaseIn;
	else if (name == "easeout") *curve = GrowthCurve::EaseOut;
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant growth curve must be Linear, SmoothStep, EaseIn, or EaseOut.";
		}
		return false;
	}
	return true;
}

bool parse_tropism_type(
	const GeometryExpression &expression,
	TropismType *type,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "gravity") *type = TropismType::Gravity;
	else if (name == "light") *type = TropismType::Light;
	else if (name == "up") *type = TropismType::Up;
	else if (name == "support") *type = TropismType::Support;
	else if (name == "moisture") *type = TropismType::Moisture;
	else if (name == "obstacleavoidance") {
		*type = TropismType::ObstacleAvoidance;
	}
	else if (name == "custom") *type = TropismType::Custom;
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant tropism type must be Gravity, Light, Up, Support, Moisture, ObstacleAvoidance, or Custom.";
		}
		return false;
	}
	return true;
}

bool parse_vine_growth_mode(
	const GeometryExpression &expression,
	VineGrowthMode *mode,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "freeclimbing") *mode = VineGrowthMode::FreeClimbing;
	else if (name == "wallclimbing") *mode = VineGrowthMode::WallClimbing;
	else if (name == "trellisclimbing") *mode = VineGrowthMode::TrellisClimbing;
	else if (name == "groundcreeping") *mode = VineGrowthMode::GroundCreeping;
	else if (name == "hanging") *mode = VineGrowthMode::Hanging;
	else if (name == "twining") *mode = VineGrowthMode::Twining;
	else if (name == "tendrilclimbing") *mode = VineGrowthMode::TendrilClimbing;
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "Vine mode must be FreeClimbing, WallClimbing, TrellisClimbing, GroundCreeping, Hanging, Twining, or TendrilClimbing.";
		}
		return false;
	}
	return true;
}

bool parse_vegetation_collision_behavior(
	const GeometryExpression &expression,
	VegetationCollisionBehavior *behavior,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "avoid") *behavior = VegetationCollisionBehavior::Avoid;
	else if (name == "seek") *behavior = VegetationCollisionBehavior::Seek;
	else if (name == "permitintersection") {
		*behavior = VegetationCollisionBehavior::PermitIntersection;
	}
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "Vine collision must be Avoid, Seek, or PermitIntersection.";
		}
		return false;
	}
	return true;
}

bool parse_surface_attachment_mode(
	const GeometryExpression &expression,
	SurfaceAttachmentMode *mode,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "contact") *mode = SurfaceAttachmentMode::Contact;
	else if (name == "offset") *mode = SurfaceAttachmentMode::Offset;
	else if (name == "twine") *mode = SurfaceAttachmentMode::Twine;
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "Vine attachment must be Contact, Offset, or Twine.";
		}
		return false;
	}
	return true;
}

bool parse_scatter_surface_face(
	const GeometryExpression &expression,
	ScatterSurfaceFace *face,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "minimumx") *face = ScatterSurfaceFace::MinimumX;
	else if (name == "maximumx") *face = ScatterSurfaceFace::MaximumX;
	else if (name == "minimumy") *face = ScatterSurfaceFace::MinimumY;
	else if (name == "maximumy") *face = ScatterSurfaceFace::MaximumY;
	else if (name == "minimumz") *face = ScatterSurfaceFace::MinimumZ;
	else if (name == "maximumz") *face = ScatterSurfaceFace::MaximumZ;
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "ScatterRegion face must be MinimumX, MaximumX, MinimumY, MaximumY, MinimumZ, or MaximumZ.";
		}
		return false;
	}
	return true;
}

bool parse_scatter_orientation_mode(
	const GeometryExpression &expression,
	ScatterOrientationMode *mode,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "surfacenormal") {
		*mode = ScatterOrientationMode::SurfaceNormal;
	}
	else if (name == "surfacenormalrandomazimuth") {
		*mode = ScatterOrientationMode::SurfaceNormalRandomAzimuth;
	}
	else if (name == "worlduprandomazimuth") {
		*mode = ScatterOrientationMode::WorldUpRandomAzimuth;
	}
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "ScatterRegion orientation must be SurfaceNormal, SurfaceNormalRandomAzimuth, or WorldUpRandomAzimuth.";
		}
		return false;
	}
	return true;
}

bool parse_collision_layer(
	const GeometryExpression &expression,
	CollisionLayer *layer,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "structure") *layer = CollisionLayer::Structure;
	else if (name == "envelope") *layer = CollisionLayer::Envelope;
	else if (name == "interior") *layer = CollisionLayer::Interior;
	else if (name == "furniture") *layer = CollisionLayer::Furniture;
	else if (name == "plumbing") *layer = CollisionLayer::Plumbing;
	else if (name == "hvac") *layer = CollisionLayer::Hvac;
	else if (name == "electrical") *layer = CollisionLayer::Electrical;
	else if (name == "equipment") *layer = CollisionLayer::Equipment;
	else if (name == "terrain") *layer = CollisionLayer::Terrain;
	else if (name == "temporary") *layer = CollisionLayer::Temporary;
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "ScatterRegion collision layer is not recognized.";
		}
		return false;
	}
	return true;
}

bool parse_phyllotaxis_mode(
	const GeometryExpression &expression,
	PhyllotaxisMode *mode,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "alternate") *mode = PhyllotaxisMode::Alternate;
	else if (name == "opposite") *mode = PhyllotaxisMode::Opposite;
	else if (name == "decussate") *mode = PhyllotaxisMode::Decussate;
	else if (name == "whorled") *mode = PhyllotaxisMode::Whorled;
	else if (name == "spiral") *mode = PhyllotaxisMode::Spiral;
	else if (name == "rosette") *mode = PhyllotaxisMode::Rosette;
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "Phyllotaxis mode must be Alternate, Opposite, Decussate, Whorled, Spiral, or Rosette.";
		}
		return false;
	}
	return true;
}

bool parse_inflorescence_kind(
	const GeometryExpression &expression,
	InflorescenceKind *kind,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "raceme") *kind = InflorescenceKind::Raceme;
	else if (name == "spike") *kind = InflorescenceKind::Spike;
	else if (name == "panicle") *kind = InflorescenceKind::Panicle;
	else if (name == "umbel") *kind = InflorescenceKind::Umbel;
	else if (name == "corymb") *kind = InflorescenceKind::Corymb;
	else if (name == "head") *kind = InflorescenceKind::Head;
	else {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Inflorescence kind must be Raceme, Spike, Panicle, Umbel, Corymb, or Head.";
		}
		return false;
	}
	return true;
}

bool parse_vegetation_organ_type(
	const GeometryExpression &expression,
	VegetationOrganType *organ_type,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "bud") *organ_type = VegetationOrganType::Bud;
	else if (name == "leaf") *organ_type = VegetationOrganType::Leaf;
	else if (name == "petal") *organ_type = VegetationOrganType::Petal;
	else if (name == "flower") *organ_type = VegetationOrganType::Flower;
	else if (name == "fruit") *organ_type = VegetationOrganType::Fruit;
	else if (name == "thorn") *organ_type = VegetationOrganType::Thorn;
	else {
		if (diagnostic != nullptr) {
			*diagnostic =
				"OrganArray organ must be Bud, Leaf, Petal, Flower, Fruit, or Thorn.";
		}
		return false;
	}
	return true;
}

bool parse_vegetation_organ_array_host(
	const GeometryExpression &expression,
	VegetationOrganArrayHost *host,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "stem") *host = VegetationOrganArrayHost::Stem;
	else if (name == "branch") *host = VegetationOrganArrayHost::Branch;
	else if (name == "flowerhead") {
		*host = VegetationOrganArrayHost::FlowerHead;
	}
	else if (name == "surface") *host = VegetationOrganArrayHost::Surface;
	else {
		if (diagnostic != nullptr) {
			*diagnostic =
				"OrganArray host must be Stem, Branch, FlowerHead, or Surface.";
		}
		return false;
	}
	return true;
}

bool parse_vegetation_organ_orientation(
	const GeometryExpression &expression,
	VegetationOrganOrientation *orientation,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "outward") {
		*orientation = VegetationOrganOrientation::Outward;
	}
	else if (name == "alonghost") {
		*orientation = VegetationOrganOrientation::AlongHost;
	}
	else if (name == "worldup") {
		*orientation = VegetationOrganOrientation::WorldUp;
	}
	else if (name == "worlddown") {
		*orientation = VegetationOrganOrientation::WorldDown;
	}
	else if (name == "surfacenormal") {
		*orientation = VegetationOrganOrientation::SurfaceNormal;
	}
	else {
		if (diagnostic != nullptr) {
			*diagnostic =
				"OrganArray orientation must be Outward, AlongHost, WorldUp, WorldDown, or SurfaceNormal.";
		}
		return false;
	}
	return true;
}

bool parse_geometry_detail_level(
	const GeometryExpression &expression,
	GeometryDetailLevel *detail_level,
	std::string *diagnostic)
{
	const std::string name = lowercase_copy(expression.sourceText());
	if (name == "lod0" || name == "bounds") {
		*detail_level = GeometryDetailLevel::Bounds;
	}
	else if (name == "lod1" || name == "coarseshape") {
		*detail_level = GeometryDetailLevel::CoarseShape;
	}
	else if (name == "lod2" || name == "assembly") {
		*detail_level = GeometryDetailLevel::Assembly;
	}
	else if (name == "lod3" || name == "component") {
		*detail_level = GeometryDetailLevel::Component;
	}
	else if (name == "lod4" || name == "constructiondetail") {
		*detail_level = GeometryDetailLevel::ConstructionDetail;
	}
	else if (name == "lod5" || name == "fastenersandseals") {
		*detail_level = GeometryDetailLevel::FastenersAndSeals;
	}
	else {
		if (diagnostic != nullptr) {
			*diagnostic = "Geometry detail must be LOD0 through LOD5 or a canonical geometry detail name.";
		}
		return false;
	}
	return true;
}

bool parse_topology(const GeometryExpression &expression,
	                ShapeTopology *topology,
	                std::string *diagnostic)
{
	if (expression.sourceText() == "surface") {
		*topology = ShapeTopology::Surface;
		return true;
	}
	if (expression.sourceText() == "solid") {
		*topology = ShapeTopology::Solid;
		return true;
	}
	if (expression.sourceText() == "shell") {
		*topology = ShapeTopology::Shell;
		return true;
	}
	if (diagnostic != nullptr) {
		*diagnostic = "Topology must be 'surface', 'solid', or 'shell'.";
	}
	return false;
}

bool parse_mapping(const GeometryExpression &expression,
	               ShapeFamily family,
	               ShapeMappingMode *mapping,
	               std::string *diagnostic)
{
	if (expression.sourceText() == "triplanar") {
		*mapping = ShapeMappingMode::Triplanar;
		return true;
	}
	if (family == ShapeFamily::Cylinder &&
	    expression.sourceText() == "cylindrical") {
		*mapping = ShapeMappingMode::Cylindrical;
		return true;
	}
	if (family == ShapeFamily::Sphere &&
	    expression.sourceText() == "spherical") {
		*mapping = ShapeMappingMode::Spherical;
		return true;
	}
	if (diagnostic != nullptr) {
		*diagnostic = family == ShapeFamily::Cylinder
			? "Cylinder mapping must be 'triplanar' or 'cylindrical'."
			: "Sphere mapping must be 'triplanar' or 'spherical'.";
	}
	return false;
}

bool parse_closure(const ShapeOptionSyntax &option,
	               ShapeClosurePolicy *closure,
	               std::string *diagnostic)
{
	bool saw_all = false;
	bool saw_none = false;
	ShapeClosurePolicy parsed = ShapeClosurePolicy::createNone();
	std::set<std::string> names;
	for (const GeometryExpression &argument : option.arguments()) {
		const std::string &name = argument.sourceText();
		if (!names.insert(name).second) {
			if (diagnostic != nullptr) {
				*diagnostic = "Closure component '" + name + "' is duplicated.";
			}
			return false;
		}
		if (name == "all") saw_all = true;
		else if (name == "none") saw_none = true;
		else if (name == "axial") parsed.includeAxial();
		else if (name == "angular") parsed.includeAngular();
		else if (name == "clip") parsed.includeClip();
		else if (name == "rim") parsed.includeRim();
		else {
			if (diagnostic != nullptr) {
				*diagnostic = "Unknown closure component '" + name + "'.";
			}
			return false;
		}
	}
	if ((saw_all || saw_none) && option.arguments().size() != 1) {
		if (diagnostic != nullptr) {
			*diagnostic = "Closure 'all' and 'none' cannot be combined with other components.";
		}
		return false;
	}
	*closure = saw_all ? ShapeClosurePolicy::createAll()
	                   : (saw_none ? ShapeClosurePolicy::createNone() : parsed);
	return true;
}

bool claim_single_option(const ShapeOptionSyntax &option,
	                    std::set<std::string> *seen,
	                    std::string *diagnostic)
{
	if (option.optionName() == "clip") return true;
	if (!seen->insert(option.optionName()).second) {
		if (diagnostic != nullptr) {
			*diagnostic = "Shape option '" + option.optionName() +
			              "' is declared more than once.";
		}
		return false;
	}
	return true;
}

bool evaluate_clip(const ShapeOptionSyntax &option,
	               PlaneClipSemantic semantic,
	               const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator,
	               PlaneClipSpecification *clip,
	               std::string *diagnostic)
{
	float nx = 0.0f;
	float ny = 0.0f;
	float nz = 0.0f;
	float offset = 0.0f;
	if (!evaluate_number(option.arguments()[0], "clip normal X", evaluator, &nx) ||
	    !evaluate_number(option.arguments()[1], "clip normal Y", evaluator, &ny) ||
	    !evaluate_number(option.arguments()[2], "clip normal Z", evaluator, &nz) ||
	    !evaluate_number(option.arguments()[3], "clip offset", evaluator, &offset)) {
		return false;
	}
	ClipRetainedSide side;
	if (!parse_side(option.arguments()[4], &side, diagnostic)) {
		return false;
	}
	*clip = PlaneClipSpecification(glm::vec3(nx, ny, nz), offset, side, semantic);
	return true;
}

ShapeSpecificationEvaluationResult evaluate_cylinder(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	CylinderShapeSpecificationCandidate candidate;
	std::set<std::string> seen;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		if (!claim_single_option(option, &seen, &result.diagnostic)) return result;
		const auto &arguments = option.arguments();
		if (option.optionName() == "radial") {
			if (seen.count("wall") != 0) {
				result.diagnostic = "Options 'radial' and 'wall' are mutually exclusive.";
				return result;
			}
			if (!evaluate_number(arguments[0], "inner radius", evaluator,
			                     &candidate.radial_minimum) ||
			    !evaluate_number(arguments[1], "outer radius", evaluator,
			                     &candidate.radial_maximum)) return result;
			candidate.radial_was_explicit = true;
		}
		else if (option.optionName() == "wall") {
			if (seen.count("radial") != 0) {
				result.diagnostic = "Options 'radial' and 'wall' are mutually exclusive.";
				return result;
			}
			float thickness = 0.0f;
			if (!evaluate_number(arguments[0], "wall thickness", evaluator,
			                     &thickness)) return result;
			candidate.radial_minimum = 1.0f - thickness;
			candidate.radial_maximum = 1.0f;
			candidate.wall_was_explicit = true;
			if (!candidate.topology_was_explicit) candidate.topology = ShapeTopology::Shell;
		}
		else if (option.optionName() == "axial") {
			if (!evaluate_number(arguments[0], "axial minimum", evaluator,
			                     &candidate.axial_minimum) ||
			    !evaluate_number(arguments[1], "axial maximum", evaluator,
			                     &candidate.axial_maximum)) return result;
		}
		else if (option.optionName() == "azimuth") {
			if (!evaluate_number(arguments[0], "azimuth start", evaluator,
			                     &candidate.azimuth_start_degrees) ||
			    !evaluate_number(arguments[1], "azimuth sweep", evaluator,
			                     &candidate.azimuth_sweep_degrees)) return result;
		}
		else if (option.optionName() == "chord") {
			float angle_degrees = 0.0f;
			float offset = 0.0f;
			if (!evaluate_number(arguments[0], "chord normal angle", evaluator,
			                     &angle_degrees) ||
			    !evaluate_number(arguments[1], "chord offset", evaluator,
			                     &offset)) return result;
			ClipRetainedSide side;
			if (!parse_side(arguments[2], &side, &result.diagnostic)) return result;
			const float radians = angle_degrees * 3.14159265358979323846f / 180.0f;
			candidate.clips.emplace_back(
				glm::vec3(std::cos(radians), 0.0f, std::sin(radians)),
				offset,
				side,
				PlaneClipSemantic::CylinderChord);
		}
		else if (option.optionName() == "clip") {
			PlaneClipSpecification clip(glm::vec3(0.0f), 0.0f,
			                            ClipRetainedSide::Positive);
			if (!evaluate_clip(option, PlaneClipSemantic::Explicit, evaluator,
			                   &clip, &result.diagnostic)) return result;
			candidate.clips.push_back(clip);
		}
		else if (option.optionName() == "topology") {
			if (!parse_topology(arguments[0], &candidate.topology,
			                    &result.diagnostic)) return result;
			candidate.topology_was_explicit = true;
		}
		else if (option.optionName() == "close") {
			if (!parse_closure(option, &candidate.closure_policy,
			                   &result.diagnostic)) return result;
		}
		else if (option.optionName() == "segments") {
			if (!evaluate_integer(arguments[0], "circumferential segments", evaluator,
			                      &candidate.circumferential_segments,
			                      &result.diagnostic)) return result;
			if (arguments.size() > 1 &&
			    !evaluate_integer(arguments[1], "axial segments", evaluator,
			                      &candidate.axial_segments,
			                      &result.diagnostic)) return result;
		}
		else if (option.optionName() == "mapping") {
			if (!parse_mapping(arguments[0], ShapeFamily::Cylinder,
			                   &candidate.mapping_mode, &result.diagnostic)) return result;
		}
		else {
			result.diagnostic = "Option '" + option.optionName() +
			                    "' is not supported by Cylinder.";
			return result;
		}
	}

	ShapeSpecificationValidator validator;
	result.specification = validator.validateCylinder(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_sphere(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	SphereShapeSpecificationCandidate candidate;
	std::set<std::string> seen;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		if (!claim_single_option(option, &seen, &result.diagnostic)) return result;
		const auto &arguments = option.arguments();
		if (option.optionName() == "radial") {
			if (seen.count("wall") != 0) {
				result.diagnostic = "Options 'radial' and 'wall' are mutually exclusive.";
				return result;
			}
			if (!evaluate_number(arguments[0], "inner radius", evaluator,
			                     &candidate.radial_minimum) ||
			    !evaluate_number(arguments[1], "outer radius", evaluator,
			                     &candidate.radial_maximum)) return result;
			candidate.radial_was_explicit = true;
		}
		else if (option.optionName() == "wall") {
			if (seen.count("radial") != 0) {
				result.diagnostic = "Options 'radial' and 'wall' are mutually exclusive.";
				return result;
			}
			float thickness = 0.0f;
			if (!evaluate_number(arguments[0], "wall thickness", evaluator,
			                     &thickness)) return result;
			candidate.radial_minimum = 1.0f - thickness;
			candidate.radial_maximum = 1.0f;
			candidate.wall_was_explicit = true;
			if (!candidate.topology_was_explicit) candidate.topology = ShapeTopology::Shell;
		}
		else if (option.optionName() == "polar") {
			if (!evaluate_number(arguments[0], "polar minimum", evaluator,
			                     &candidate.polar_minimum_degrees) ||
			    !evaluate_number(arguments[1], "polar maximum", evaluator,
			                     &candidate.polar_maximum_degrees)) return result;
		}
		else if (option.optionName() == "azimuth") {
			if (!evaluate_number(arguments[0], "azimuth start", evaluator,
			                     &candidate.azimuth_start_degrees) ||
			    !evaluate_number(arguments[1], "azimuth sweep", evaluator,
			                     &candidate.azimuth_sweep_degrees)) return result;
		}
		else if (option.optionName() == "clip") {
			PlaneClipSpecification clip(glm::vec3(0.0f), 0.0f,
			                            ClipRetainedSide::Positive);
			if (!evaluate_clip(option, PlaneClipSemantic::Explicit, evaluator,
			                   &clip, &result.diagnostic)) return result;
			candidate.clips.push_back(clip);
		}
		else if (option.optionName() == "slab") {
			float nx = 0.0f;
			float ny = 0.0f;
			float nz = 0.0f;
			float minimum_offset = 0.0f;
			float maximum_offset = 0.0f;
			if (!evaluate_number(arguments[0], "slab normal X", evaluator, &nx) ||
			    !evaluate_number(arguments[1], "slab normal Y", evaluator, &ny) ||
			    !evaluate_number(arguments[2], "slab normal Z", evaluator, &nz) ||
			    !evaluate_number(arguments[3], "slab minimum offset", evaluator,
			                     &minimum_offset) ||
			    !evaluate_number(arguments[4], "slab maximum offset", evaluator,
			                     &maximum_offset)) return result;
			if (minimum_offset >= maximum_offset) {
				result.diagnostic = "Sphere slab requires minimum offset < maximum offset.";
				return result;
			}
			const glm::vec3 normal(nx, ny, nz);
			candidate.clips.emplace_back(normal, minimum_offset,
				ClipRetainedSide::Positive, PlaneClipSemantic::SphereSlabMinimum);
			candidate.clips.emplace_back(normal, maximum_offset,
				ClipRetainedSide::Negative, PlaneClipSemantic::SphereSlabMaximum);
		}
		else if (option.optionName() == "topology") {
			if (!parse_topology(arguments[0], &candidate.topology,
			                    &result.diagnostic)) return result;
			candidate.topology_was_explicit = true;
		}
		else if (option.optionName() == "close") {
			if (!parse_closure(option, &candidate.closure_policy,
			                   &result.diagnostic)) return result;
		}
		else if (option.optionName() == "segments") {
			if (!evaluate_integer(arguments[0], "azimuth segments", evaluator,
			                      &candidate.azimuth_segments,
			                      &result.diagnostic)) return result;
			if (arguments.size() > 1 &&
			    !evaluate_integer(arguments[1], "polar segments", evaluator,
			                      &candidate.polar_segments,
			                      &result.diagnostic)) return result;
		}
		else if (option.optionName() == "mapping") {
			if (!parse_mapping(arguments[0], ShapeFamily::Sphere,
			                   &candidate.mapping_mode, &result.diagnostic)) return result;
		}
		else {
			result.diagnostic = "Option '" + option.optionName() +
			                    "' is not supported by Sphere.";
			return result;
		}
	}

	ShapeSpecificationValidator validator;
	result.specification = validator.validateSphere(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_tapered_sweep(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	TaperedSweepShapeSpecificationCandidate candidate;
	std::set<std::string> seen;
	bool samples_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		if (!claim_single_option(option, &seen, &result.diagnostic)) return result;
		const auto &arguments = option.arguments();
		if (option.optionName() == "samples") {
			samples_seen = true;
			for (std::size_t index = 0; index < arguments.size(); index += 4u) {
				glm::vec3 point(0.0f);
				float radius = 0.0f;
				if (!evaluate_number(
						arguments[index], "TaperedSweep sample X", evaluator, &point.x) ||
				    !evaluate_number(
						arguments[index + 1u], "TaperedSweep sample Y", evaluator, &point.y) ||
				    !evaluate_number(
						arguments[index + 2u], "TaperedSweep sample Z", evaluator, &point.z) ||
				    !evaluate_number(
						arguments[index + 3u], "TaperedSweep sample radius", evaluator, &radius)) {
					return result;
				}
				candidate.path_points.push_back(point);
				candidate.radii.push_back(radius);
			}
		}
		else if (option.optionName() == "up") {
			if (!evaluate_number(arguments[0], "TaperedSweep up X", evaluator,
			                     &candidate.up_hint.x) ||
			    !evaluate_number(arguments[1], "TaperedSweep up Y", evaluator,
			                     &candidate.up_hint.y) ||
			    !evaluate_number(arguments[2], "TaperedSweep up Z", evaluator,
			                     &candidate.up_hint.z)) {
				return result;
			}
		}
		else if (option.optionName() == "segments") {
			if (!evaluate_integer(
					arguments[0], "TaperedSweep segments", evaluator,
					&candidate.circumferential_segments, &result.diagnostic)) {
				return result;
			}
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.cap_policy, &result.diagnostic)) {
				return result;
			}
		}
		else {
			result.diagnostic = "Option '" + option.optionName() +
			                    "' is not supported by TaperedSweep.";
			return result;
		}
	}
	if (!samples_seen) {
		result.diagnostic = "TaperedSweep requires samples(x y z radius ...).";
		return result;
	}
	result.specification = TaperedSweepSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_branch_junction(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	BranchJunctionShapeSpecificationCandidate candidate;
	std::set<std::string> seen;
	bool core_seen = false;
	bool parent_seen = false;
	std::optional<BranchJunctionArmSpecification> parent_arm;
	std::vector<BranchJunctionArmSpecification> child_arms;

	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const std::string &option_name = option.optionName();
		if (option_name != "child" &&
		    !claim_single_option(option, &seen, &result.diagnostic)) {
			return result;
		}
		const auto &arguments = option.arguments();
		if (option_name == "core") {
			core_seen = true;
			if (!evaluate_number(
					arguments[0], "BranchJunction core radius", evaluator,
					&candidate.core_radius) ||
			    !evaluate_number(
					arguments[1], "BranchJunction bulge scale", evaluator,
					&candidate.bulge_scale)) {
				return result;
			}
		}
		else if (option_name == "parent" || option_name == "child") {
			glm::vec3 direction(0.0f);
			float radius = 0.0f;
			float transition_length = 0.0f;
			const std::string purpose = option_name == "parent"
				? "BranchJunction parent" : "BranchJunction child";
			if (!evaluate_number(arguments[0], purpose + " direction X", evaluator,
			                     &direction.x) ||
			    !evaluate_number(arguments[1], purpose + " direction Y", evaluator,
			                     &direction.y) ||
			    !evaluate_number(arguments[2], purpose + " direction Z", evaluator,
			                     &direction.z) ||
			    !evaluate_number(arguments[3], purpose + " radius", evaluator,
			                     &radius) ||
			    !evaluate_number(arguments[4], purpose + " transition length", evaluator,
			                     &transition_length)) {
				return result;
			}
			BranchJunctionArmSpecification arm(
				option_name == "parent" ? BranchJunctionArmRole::Parent
				                         : BranchJunctionArmRole::Child,
				direction, radius, transition_length);
			if (option_name == "parent") {
				parent_seen = true;
				parent_arm = std::move(arm);
			}
			else {
				child_arms.push_back(std::move(arm));
			}
		}
		else if (option_name == "segments") {
			if (!evaluate_integer(
					arguments[0], "BranchJunction segments", evaluator,
					&candidate.circumferential_segments, &result.diagnostic)) {
				return result;
			}
		}
		else {
			result.diagnostic = "Option '" + option_name +
			                    "' is not supported by BranchJunction.";
			return result;
		}
	}

	if (!core_seen) {
		result.diagnostic = "BranchJunction requires core(radius bulgeScale).";
		return result;
	}
	if (!parent_seen || !parent_arm.has_value()) {
		result.diagnostic =
			"BranchJunction requires parent(dx dy dz radius transitionLength).";
		return result;
	}
	if (child_arms.size() < 2u) {
		result.diagnostic =
			"BranchJunction requires at least two child(dx dy dz radius transitionLength) arms.";
		return result;
	}

	candidate.arms.reserve(child_arms.size() + 1u);
	candidate.arms.push_back(std::move(*parent_arm));
	for (BranchJunctionArmSpecification &child_arm : child_arms) {
		candidate.arms.push_back(std::move(child_arm));
	}
	result.specification = BranchJunctionSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_botanical_blade(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	BotanicalBladeShapeSpecificationCandidate candidate;
	candidate.kind = descriptor.shapeName() == "PetalBlade"
		? BotanicalBladeKind::Petal
		: BotanicalBladeKind::Leaf;
	std::set<std::string> seen;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		if (!claim_single_option(option, &seen, &result.diagnostic)) return result;
		const auto &arguments = option.arguments();
		if (option.optionName() == "profile") {
			if (!parse_botanical_blade_profile(
					arguments[0], &candidate.profile, &result.diagnostic)) {
				return result;
			}
		}
		else if (option.optionName() == "length") {
			if (!evaluate_number(
					arguments[0], "botanical blade length", evaluator,
					&candidate.length)) return result;
		}
		else if (option.optionName() == "width") {
			if (!evaluate_number(
					arguments[0], "botanical blade width", evaluator,
					&candidate.maximum_width)) return result;
		}
		else if (option.optionName() == "curvature") {
			if (!evaluate_number(
					arguments[0], "botanical blade curvature", evaluator,
					&candidate.longitudinal_curvature)) return result;
		}
		else if (option.optionName() == "camber") {
			if (!evaluate_number(
					arguments[0], "botanical blade camber", evaluator,
					&candidate.camber)) return result;
		}
		else if (option.optionName() == "twist") {
			if (!evaluate_number(
					arguments[0], "botanical blade twist", evaluator,
					&candidate.twist_degrees)) return result;
		}
		else if (option.optionName() == "thickness") {
			if (!evaluate_number(
					arguments[0], "botanical blade thickness", evaluator,
					&candidate.thickness)) return result;
		}
		else if (option.optionName() == "widthPower") {
			if (!evaluate_number(
					arguments[0], "botanical blade width power", evaluator,
					&candidate.width_power)) return result;
		}
		else if (option.optionName() == "segments") {
			if (!evaluate_integer(
					arguments[0], "botanical blade longitudinal segments", evaluator,
					&candidate.longitudinal_segments, &result.diagnostic) ||
			    !evaluate_integer(
					arguments[1], "botanical blade lateral segments", evaluator,
					&candidate.lateral_segments, &result.diagnostic)) {
				return result;
			}
		}
		else {
			result.diagnostic = "Option '" + option.optionName() +
			                    "' is not supported by " + descriptor.shapeName() + ".";
			return result;
		}
	}
	result.specification = BotanicalBladeSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_plant(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	const PlantSpeciesSpecification *catalog_species = nullptr;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		if (option.optionName() != "species") continue;
		catalog_species = PlantSpeciesCatalog().find(
			option.arguments()[0].sourceText());
		if (catalog_species == nullptr) {
			result.diagnostic = "Unknown plant species '" +
			                    option.arguments()[0].sourceText() + "'.";
			return result;
		}
	}
	if (catalog_species == nullptr) {
		result.diagnostic = "Plant requires species(name).";
		return result;
	}

	PlantArchitecture architecture = catalog_species->architecture();
	const PlantBranchingSpecification &base_branching = catalog_species->branching();
	float primary_axis_length = base_branching.primaryAxisLength();
	float base_radius = base_branching.baseRadius();
	float tip_radius = base_branching.tipRadius();
	int primary_internode_count = base_branching.primaryInternodeCount();
	int maximum_branch_order = base_branching.maximumBranchOrder();
	int lateral_branches_per_node = base_branching.lateralBranchesPerNode();
	int branch_every_n_internodes = base_branching.branchEveryNInternodes();
	float branch_angle_degrees = base_branching.branchAngleDegrees();
	float azimuth_divergence_degrees = base_branching.azimuthDivergenceDegrees();
	float branch_length_fraction = base_branching.branchLengthFraction();
	float branch_length_falloff = base_branching.branchLengthFalloff();
	float continuation_radius_ratio = base_branching.continuationRadiusRatio();
	float radius_conservation_gamma = base_branching.radiusConservationGamma();
	float direction_variation_degrees = base_branching.directionVariationDegrees();
	float length_variation_fraction = base_branching.lengthVariationFraction();
	float upward_tropism_weight = base_branching.upwardTropismWeight();

	const PlantLeafSpecification &base_leaf = catalog_species->leaf();
	BotanicalBladeProfile leaf_profile = base_leaf.profile();
	float leaf_length = base_leaf.length();
	float leaf_width = base_leaf.width();
	float leaf_camber = base_leaf.camber();
	float leaf_twist_degrees = base_leaf.twistDegrees();
	float leaf_thickness = base_leaf.thickness();
	int leaf_minimum_branch_order = base_leaf.minimumBranchOrder();
	float petiole_length = base_leaf.petiole().length();
	float petiole_base_radius = base_leaf.petiole().baseRadius();
	float petiole_tip_radius = base_leaf.petiole().tipRadius();
	const PlantCompoundLeafSpecification &base_compound_leaf =
		base_leaf.compoundLeaf();
	bool compound_leaf_enabled = base_compound_leaf.isEnabled();
	int leaflet_node_count = base_compound_leaf.leafletNodeCount();
	float rachis_length = base_compound_leaf.rachisLength();
	float rachis_base_radius = base_compound_leaf.rachisBaseRadius();
	float rachis_tip_radius = base_compound_leaf.rachisTipRadius();
	PhyllotaxisMode compound_leaf_pattern = base_compound_leaf.pattern();
	float compound_leaf_divergence = base_compound_leaf.divergenceDegrees();
	float compound_leaf_up_bias = base_compound_leaf.orientationUpBias();
	float leaflet_scale = base_compound_leaf.leafletScale();

	const PhyllotaxisSpecification &base_phyllotaxis = base_leaf.phyllotaxis();
	PhyllotaxisMode phyllotaxis_mode = base_phyllotaxis.mode();
	float phyllotaxis_divergence = base_phyllotaxis.divergenceDegrees();
	float phyllotaxis_internode_length = base_phyllotaxis.internodeLength();
	int phyllotaxis_organs_per_node = base_phyllotaxis.organsPerNode();
	float phyllotaxis_phase = base_phyllotaxis.phaseDegrees();
	float phyllotaxis_radial_offset = base_phyllotaxis.radialOffset();
	float phyllotaxis_up_bias = base_phyllotaxis.orientationUpBias();

	const PlantFlowerSpecification &base_flower = catalog_species->flower();
	bool flower_enabled = base_flower.isEnabled();
	BotanicalBladeProfile petal_profile = base_flower.petalProfile();
	float petal_length = base_flower.petalLength();
	float petal_width = base_flower.petalWidth();
	float petal_curvature = base_flower.longitudinalCurvature();
	float petal_camber = base_flower.camber();
	float petal_twist_degrees = base_flower.twistDegrees();
	float petal_thickness = base_flower.thickness();
	float petal_width_power = base_flower.widthPower();
	int whorl_count = base_flower.petalWhorl().organCount();
	float whorl_radius = base_flower.petalWhorl().radius();
	float whorl_phase = base_flower.petalWhorl().phaseDegrees();
	float whorl_tilt = base_flower.petalWhorl().tiltDegrees();
	FlowerHeadSpecification flower_head = base_flower.flowerHead();
	InflorescenceSpecification inflorescence = base_flower.inflorescence();
	const PlantFruitSpecification &base_fruit = catalog_species->fruit();
	bool fruit_enabled = base_fruit.isEnabled();
	float fruit_height = base_fruit.height();
	float fruit_radius = base_fruit.maximumRadius();
	float fruit_shoulder = base_fruit.shoulderFraction();
	float fruit_fullness = base_fruit.fullness();
	int fruit_radial_segments = base_fruit.radialSegments();
	int fruit_profile_segments = base_fruit.profileSegments();
	std::vector<PlantOrganArraySpecification> organ_arrays =
		catalog_species->organArrays();

	float flowering_age = catalog_species->floweringAge();
	float mature_age = catalog_species->matureAge();
	float age = mature_age;
	bool age_was_explicit = false;
	PlantDevelopmentState development_state = PlantDevelopmentState::Mature;
	std::uint64_t deterministic_seed = 1u;
	PlantTopologyGenerationMethod topology_generation_method =
		PlantTopologyGenerationMethod::RuleBranching;
	bool l_system_axiom_declared = false;
	std::vector<LSystemSymbol> l_system_axiom;
	std::vector<LSystemProductionRule> l_system_rules;
	bool l_system_parameters_declared = false;
	int l_system_iteration_count = 0;
	float l_system_turn_angle_degrees = 25.0f;
	float l_system_step_length = 0.2f;
	float l_system_base_radius = 0.05f;
	float l_system_terminal_radius = 0.006f;
	float l_system_radius_gamma = 2.0f;
	std::optional<CrownVolumeSpecification> crown_volume;
	bool custom_crown_declared = false;
	float custom_crown_sample_radius = 0.0f;
	std::vector<glm::vec3> custom_crown_samples;
	bool space_colonization_declared = false;
	float colonization_influence_radius = 0.0f;
	float colonization_kill_radius = 0.0f;
	float colonization_step_length = 0.0f;
	int colonization_maximum_iterations = 0;
	int colonization_attraction_point_count = 0;
	float colonization_radius_decay = 0.0f;
	float colonization_minimum_radius = 0.0f;
	float colonization_radius_gamma = 0.0f;
	std::vector<VegetationObstacleBoundary> crown_obstacles;
	std::optional<PlantGrowthSpecification> growth;
	std::vector<TropismInfluence> tropism_influences;
	GeometryDetailLevel detail_level = GeometryDetailLevel::Component;
	std::set<std::string> seen;

	for (const ShapeOptionSyntax &option : descriptor.options()) {
		if (option.optionName() != "tropism" &&
		    option.optionName() != "organArray" &&
		    option.optionName() != "lRule" &&
		    option.optionName() != "crownSample" &&
		    option.optionName() != "crownObstacle" &&
		    !claim_single_option(option, &seen, &result.diagnostic)) return result;
		const auto &arguments = option.arguments();
		if (option.optionName() == "species") {
			continue;
		}
		if (option.optionName() == "architecture") {
			if (!parse_plant_architecture(
					arguments[0], &architecture, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "age") {
			if (!evaluate_number(arguments[0], "Plant age", evaluator, &age)) {
				return result;
			}
			age_was_explicit = true;
		}
		else if (option.optionName() == "state") {
			if (!parse_plant_development_state(
					arguments[0], &development_state, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "seed") {
			int evaluated_seed = 0;
			if (!evaluate_integer(
					arguments[0], "Plant seed", evaluator, &evaluated_seed,
					&result.diagnostic)) return result;
			if (evaluated_seed < 0) {
				result.diagnostic = "Plant seed must be non-negative.";
				return result;
			}
			deterministic_seed = static_cast<std::uint64_t>(evaluated_seed);
		}
		else if (option.optionName() == "generation") {
			if (!parse_plant_topology_generation_method(
					arguments[0], &topology_generation_method,
					&result.diagnostic)) return result;
		}
		else if (option.optionName() == "lAxiom") {
			l_system_axiom_declared = true;
			l_system_axiom.clear();
			for (const GeometryExpression &argument : arguments) {
				LSystemSymbol symbol = LSystemSymbol::Forward;
				if (!parse_l_system_symbol(
						argument, &symbol, &result.diagnostic)) return result;
				l_system_axiom.push_back(symbol);
			}
		}
		else if (option.optionName() == "lRule") {
			LSystemSymbol predecessor = LSystemSymbol::Forward;
			if (!parse_l_system_symbol(
					arguments[0], &predecessor, &result.diagnostic)) return result;
			std::vector<LSystemSymbol> successor;
			successor.reserve(arguments.size() - 1u);
			for (std::size_t index = 1u; index < arguments.size(); ++index) {
				LSystemSymbol symbol = LSystemSymbol::Forward;
				if (!parse_l_system_symbol(
						arguments[index], &symbol, &result.diagnostic)) return result;
				successor.push_back(symbol);
			}
			l_system_rules.emplace_back(predecessor, std::move(successor));
		}
		else if (option.optionName() == "lSystem") {
			l_system_parameters_declared = true;
			if (!evaluate_integer(
					arguments[0], "Plant L-system iterations", evaluator,
					&l_system_iteration_count, &result.diagnostic) ||
			    !evaluate_number(
					arguments[1], "Plant L-system turn angle", evaluator,
					&l_system_turn_angle_degrees) ||
			    !evaluate_number(
					arguments[2], "Plant L-system step length", evaluator,
					&l_system_step_length) ||
			    !evaluate_number(
					arguments[3], "Plant L-system base radius", evaluator,
					&l_system_base_radius) ||
			    !evaluate_number(
					arguments[4], "Plant L-system terminal radius", evaluator,
					&l_system_terminal_radius) ||
			    !evaluate_number(
					arguments[5], "Plant L-system radius gamma", evaluator,
					&l_system_radius_gamma)) return result;
		}
		else if (option.optionName() == "trunk") {
			if (!evaluate_number(arguments[0], "Plant trunk length", evaluator,
			                     &primary_axis_length) ||
			    !evaluate_number(arguments[1], "Plant trunk base radius", evaluator,
			                     &base_radius) ||
			    !evaluate_number(arguments[2], "Plant trunk tip radius", evaluator,
			                     &tip_radius) ||
			    !evaluate_integer(arguments[3], "Plant trunk internodes", evaluator,
			                      &primary_internode_count, &result.diagnostic)) {
				return result;
			}
		}
		else if (option.optionName() == "branching") {
			if (!evaluate_integer(arguments[0], "Plant maximum branch order", evaluator,
			                      &maximum_branch_order, &result.diagnostic) ||
			    !evaluate_integer(arguments[1], "Plant lateral branches per node", evaluator,
			                      &lateral_branches_per_node, &result.diagnostic) ||
			    !evaluate_integer(arguments[2], "Plant branch interval", evaluator,
			                      &branch_every_n_internodes, &result.diagnostic) ||
			    !evaluate_number(arguments[3], "Plant branch angle", evaluator,
			                     &branch_angle_degrees) ||
			    !evaluate_number(arguments[4], "Plant azimuth divergence", evaluator,
			                     &azimuth_divergence_degrees) ||
			    !evaluate_number(arguments[5], "Plant branch length fraction", evaluator,
			                     &branch_length_fraction) ||
			    !evaluate_number(arguments[6], "Plant branch length falloff", evaluator,
			                     &branch_length_falloff) ||
			    !evaluate_number(arguments[7], "Plant continuation radius ratio", evaluator,
			                     &continuation_radius_ratio) ||
			    !evaluate_number(arguments[8], "Plant radius conservation gamma", evaluator,
			                     &radius_conservation_gamma) ||
			    !evaluate_number(arguments[9], "Plant direction variation", evaluator,
			                     &direction_variation_degrees) ||
			    !evaluate_number(arguments[10], "Plant length variation", evaluator,
			                     &length_variation_fraction) ||
			    !evaluate_number(arguments[11], "Plant upward tropism", evaluator,
			                     &upward_tropism_weight)) {
				return result;
			}
		}
		else if (option.optionName() == "leaf") {
			if (!parse_botanical_blade_profile(
					arguments[0], &leaf_profile, &result.diagnostic) ||
			    !evaluate_number(arguments[1], "Plant leaf length", evaluator,
			                     &leaf_length) ||
			    !evaluate_number(arguments[2], "Plant leaf width", evaluator,
			                     &leaf_width) ||
			    !evaluate_number(arguments[3], "Plant leaf camber", evaluator,
			                     &leaf_camber) ||
			    !evaluate_number(arguments[4], "Plant leaf twist", evaluator,
			                     &leaf_twist_degrees) ||
			    !evaluate_number(arguments[5], "Plant leaf thickness", evaluator,
			                     &leaf_thickness) ||
			    !evaluate_integer(arguments[6], "Plant leaf minimum branch order", evaluator,
			                      &leaf_minimum_branch_order, &result.diagnostic)) {
				return result;
			}
		}
		else if (option.optionName() == "petiole") {
			if (!evaluate_number(arguments[0], "Plant petiole length", evaluator,
			                     &petiole_length) ||
			    !evaluate_number(arguments[1], "Plant petiole base radius", evaluator,
			                     &petiole_base_radius) ||
			    !evaluate_number(arguments[2], "Plant petiole tip radius", evaluator,
			                     &petiole_tip_radius)) {
				return result;
			}
		}
		else if (option.optionName() == "leafArray") {
			compound_leaf_enabled = true;
			if (!evaluate_integer(
					arguments[0], "Plant leaflet node count", evaluator,
					&leaflet_node_count, &result.diagnostic) ||
			    !evaluate_number(arguments[1], "Plant rachis length", evaluator,
			                     &rachis_length) ||
			    !evaluate_number(arguments[2], "Plant rachis base radius", evaluator,
			                     &rachis_base_radius) ||
			    !evaluate_number(arguments[3], "Plant rachis tip radius", evaluator,
			                     &rachis_tip_radius) ||
			    !parse_phyllotaxis_mode(
					arguments[4], &compound_leaf_pattern, &result.diagnostic) ||
			    !evaluate_number(arguments[5], "Plant leaflet divergence", evaluator,
			                     &compound_leaf_divergence) ||
			    !evaluate_number(arguments[6], "Plant leaflet up bias", evaluator,
			                     &compound_leaf_up_bias) ||
			    !evaluate_number(arguments[7], "Plant leaflet scale", evaluator,
			                     &leaflet_scale)) {
				return result;
			}
		}
		else if (option.optionName() == "phyllotaxis") {
			if (!parse_phyllotaxis_mode(
					arguments[0], &phyllotaxis_mode, &result.diagnostic) ||
			    !evaluate_number(arguments[1], "Plant phyllotaxis divergence", evaluator,
			                     &phyllotaxis_divergence) ||
			    !evaluate_number(arguments[2], "Plant phyllotaxis spacing", evaluator,
			                     &phyllotaxis_internode_length)) {
				return result;
			}
			if (arguments.size() > 3u &&
			    !evaluate_integer(arguments[3], "Plant organs per node", evaluator,
			                      &phyllotaxis_organs_per_node, &result.diagnostic)) return result;
			if (arguments.size() > 4u &&
			    !evaluate_number(arguments[4], "Plant phyllotaxis phase", evaluator,
			                     &phyllotaxis_phase)) return result;
			if (arguments.size() > 5u &&
			    !evaluate_number(arguments[5], "Plant phyllotaxis radial offset", evaluator,
			                     &phyllotaxis_radial_offset)) return result;
			if (arguments.size() > 6u &&
			    !evaluate_number(arguments[6], "Plant phyllotaxis up bias", evaluator,
			                     &phyllotaxis_up_bias)) return result;
		}
		else if (option.optionName() == "organArray") {
			VegetationOrganType organ_type = VegetationOrganType::Leaf;
			VegetationOrganArrayHost host = VegetationOrganArrayHost::Branch;
			VegetationOrganOrientation orientation =
				VegetationOrganOrientation::Outward;
			int count = 0;
			float spacing = 0.0f;
			float azimuth_progression_degrees = 0.0f;
			float initial_scale = 1.0f;
			float scale_falloff = 1.0f;
			float jitter_fraction = 0.0f;
			int minimum_branch_order = 0;
			if (!parse_vegetation_organ_type(
					arguments[0], &organ_type, &result.diagnostic) ||
			    !parse_vegetation_organ_array_host(
					arguments[1], &host, &result.diagnostic) ||
			    !evaluate_integer(
					arguments[2], "Plant OrganArray count", evaluator, &count,
					&result.diagnostic) ||
			    !evaluate_number(
					arguments[3], "Plant OrganArray spacing", evaluator, &spacing) ||
			    !evaluate_number(
					arguments[4], "Plant OrganArray azimuth progression", evaluator,
					&azimuth_progression_degrees) ||
			    !evaluate_number(
					arguments[5], "Plant OrganArray initial scale", evaluator,
					&initial_scale) ||
			    !evaluate_number(
					arguments[6], "Plant OrganArray scale falloff", evaluator,
					&scale_falloff) ||
			    !parse_vegetation_organ_orientation(
					arguments[7], &orientation, &result.diagnostic) ||
			    !evaluate_number(
					arguments[8], "Plant OrganArray jitter", evaluator,
					&jitter_fraction) ||
			    !evaluate_integer(
					arguments[9], "Plant OrganArray minimum branch order", evaluator,
					&minimum_branch_order, &result.diagnostic)) {
				return result;
			}
			organ_arrays.emplace_back(
				catalog_species->identifier() + ":organArray:" +
					std::to_string(organ_arrays.size()),
				organ_type, host, count, spacing, azimuth_progression_degrees,
				initial_scale, scale_falloff, orientation, jitter_fraction,
				minimum_branch_order);
		}
		else if (option.optionName() == "flower") {
			flower_enabled = true;
			if (!parse_botanical_blade_profile(
					arguments[0], &petal_profile, &result.diagnostic) ||
			    !evaluate_number(arguments[1], "Plant petal length", evaluator,
			                     &petal_length) ||
			    !evaluate_number(arguments[2], "Plant petal width", evaluator,
			                     &petal_width) ||
			    !evaluate_number(arguments[3], "Plant petal curvature", evaluator,
			                     &petal_curvature) ||
			    !evaluate_number(arguments[4], "Plant petal camber", evaluator,
			                     &petal_camber) ||
			    !evaluate_number(arguments[5], "Plant petal twist", evaluator,
			                     &petal_twist_degrees) ||
			    !evaluate_number(arguments[6], "Plant petal thickness", evaluator,
			                     &petal_thickness) ||
			    !evaluate_number(arguments[7], "Plant petal width power", evaluator,
			                     &petal_width_power)) {
				return result;
			}
		}
		else if (option.optionName() == "fruit") {
			fruit_enabled = true;
			if (!evaluate_number(arguments[0], "Plant fruit height", evaluator,
			                     &fruit_height) ||
			    !evaluate_number(arguments[1], "Plant fruit radius", evaluator,
			                     &fruit_radius) ||
			    !evaluate_number(arguments[2], "Plant fruit shoulder", evaluator,
			                     &fruit_shoulder) ||
			    !evaluate_number(arguments[3], "Plant fruit fullness", evaluator,
			                     &fruit_fullness) ||
			    !evaluate_integer(arguments[4], "Plant fruit radial segments", evaluator,
			                      &fruit_radial_segments, &result.diagnostic) ||
			    !evaluate_integer(arguments[5], "Plant fruit profile segments", evaluator,
			                      &fruit_profile_segments, &result.diagnostic)) {
				return result;
			}
		}
		else if (option.optionName() == "whorl") {
			flower_enabled = true;
			if (!evaluate_integer(arguments[0], "Plant whorl organ count", evaluator,
			                      &whorl_count, &result.diagnostic) ||
			    !evaluate_number(arguments[1], "Plant whorl radius", evaluator,
			                     &whorl_radius) ||
			    !evaluate_number(arguments[2], "Plant whorl phase", evaluator,
			                     &whorl_phase) ||
			    !evaluate_number(arguments[3], "Plant whorl tilt", evaluator,
			                     &whorl_tilt)) {
				return result;
			}
		}
		else if (option.optionName() == "flowerHead") {
			flower_enabled = true;
			int floret_count = 0;
			float head_radius = 0.0f;
			float divergence_degrees = 0.0f;
			float phase_degrees = 0.0f;
			float tilt_degrees = 0.0f;
			float scale_falloff = 0.0f;
			if (!evaluate_integer(
					arguments[0], "FlowerHead floret count", evaluator,
					&floret_count, &result.diagnostic) ||
			    !evaluate_number(
					arguments[1], "FlowerHead radius", evaluator, &head_radius) ||
			    !evaluate_number(
					arguments[2], "FlowerHead divergence", evaluator,
					&divergence_degrees) ||
			    !evaluate_number(
					arguments[3], "FlowerHead phase", evaluator, &phase_degrees) ||
			    !evaluate_number(
					arguments[4], "FlowerHead tilt", evaluator, &tilt_degrees) ||
			    !evaluate_number(
					arguments[5], "FlowerHead scale falloff", evaluator,
					&scale_falloff)) {
				return result;
			}
			flower_head = FlowerHeadSpecification(
				floret_count, head_radius, divergence_degrees, phase_degrees,
				tilt_degrees, scale_falloff);
			inflorescence = InflorescenceSpecification();
		}
		else if (option.optionName() == "inflorescence") {
			flower_enabled = true;
			InflorescenceKind kind = InflorescenceKind::Raceme;
			int flower_count = 0;
			float spacing = 0.0f;
			float radial_extent = 0.0f;
			float phase_degrees = 0.0f;
			float tilt_degrees = 0.0f;
			float scale_falloff = 0.0f;
			if (!parse_inflorescence_kind(
					arguments[0], &kind, &result.diagnostic) ||
			    !evaluate_integer(
					arguments[1], "Inflorescence flower count", evaluator,
					&flower_count, &result.diagnostic) ||
			    !evaluate_number(
					arguments[2], "Inflorescence spacing", evaluator, &spacing) ||
			    !evaluate_number(
					arguments[3], "Inflorescence radial extent", evaluator,
					&radial_extent) ||
			    !evaluate_number(
					arguments[4], "Inflorescence phase", evaluator, &phase_degrees) ||
			    !evaluate_number(
					arguments[5], "Inflorescence tilt", evaluator, &tilt_degrees) ||
			    !evaluate_number(
					arguments[6], "Inflorescence scale falloff", evaluator,
					&scale_falloff)) {
				return result;
			}
			inflorescence = InflorescenceSpecification(
				kind, flower_count, spacing, radial_extent, phase_degrees,
				tilt_degrees, scale_falloff);
			flower_head = FlowerHeadSpecification();
		}
		else if (option.optionName() == "growth") {
			float start_time = 0.0f;
			float duration = 0.0f;
			float length_initial = 0.0f;
			float radius_initial = 0.0f;
			float organ_initial = 0.0f;
			GrowthCurve length_curve = GrowthCurve::Linear;
			GrowthCurve radius_curve = GrowthCurve::Linear;
			GrowthCurve organ_curve = GrowthCurve::Linear;
			if (!evaluate_number(arguments[0], "Plant growth start time", evaluator,
			                     &start_time) ||
			    !evaluate_number(arguments[1], "Plant growth duration", evaluator,
			                     &duration) ||
			    !evaluate_number(arguments[2], "Plant initial length factor", evaluator,
			                     &length_initial) ||
			    !parse_growth_curve(arguments[3], &length_curve, &result.diagnostic) ||
			    !evaluate_number(arguments[4], "Plant initial radius factor", evaluator,
			                     &radius_initial) ||
			    !parse_growth_curve(arguments[5], &radius_curve, &result.diagnostic) ||
			    !evaluate_number(arguments[6], "Plant initial organ factor", evaluator,
			                     &organ_initial) ||
			    !parse_growth_curve(arguments[7], &organ_curve, &result.diagnostic)) {
				return result;
			}
			growth.emplace(
				start_time, duration,
				PlantGrowthChannelSpecification(length_initial, length_curve),
				PlantGrowthChannelSpecification(radius_initial, radius_curve),
				PlantGrowthChannelSpecification(organ_initial, organ_curve));
		}
		else if (option.optionName() == "tropism") {
			TropismType type = TropismType::Custom;
			glm::vec3 direction(0.0f);
			float weight = 0.0f;
			if (!parse_tropism_type(arguments[0], &type, &result.diagnostic)) {
				return result;
			}
			for (int axis = 0; axis < 3; ++axis) {
				if (!evaluate_number(
						arguments[static_cast<std::size_t>(axis + 1)],
						"Plant tropism direction", evaluator, &direction[axis])) {
					return result;
				}
			}
			if (!evaluate_number(arguments[4], "Plant tropism weight", evaluator,
			                     &weight)) return result;
			tropism_influences.emplace_back(type, direction, weight);
		}
		else if (option.optionName() == "crown") {
			if (custom_crown_declared) {
				result.diagnostic =
					"Plant accepts either crown(...) or crownCustom(...), not both.";
				return result;
			}
			CrownVolumeKind kind = CrownVolumeKind::Sphere;
			if (!parse_crown_volume_kind(
					arguments[0], &kind, &result.diagnostic)) return result;
			const std::size_t expected_argument_count =
				kind == CrownVolumeKind::Sphere
					? 5u
					: kind == CrownVolumeKind::Lobed ? 9u : 7u;
			if (arguments.size() != expected_argument_count) {
				result.diagnostic = "Plant crown " +
					std::string(crownVolumeKindName(kind)) + " requires " +
					std::to_string(expected_argument_count - 1u) +
					" numeric arguments after its kind.";
				return result;
			}
			glm::vec3 origin(0.0f);
			for (int axis = 0; axis < 3; ++axis) {
				if (!evaluate_number(
						arguments[static_cast<std::size_t>(axis + 1)],
						"Plant crown origin", evaluator, &origin[axis])) return result;
			}
			if (kind == CrownVolumeKind::Sphere) {
				float radius = 0.0f;
				if (!evaluate_number(
						arguments[4], "Plant crown radius", evaluator, &radius)) {
					return result;
				}
				crown_volume = CrownVolumeSpecification::sphere(origin, radius);
			}
			else {
				glm::vec3 dimensions(0.0f);
				for (int axis = 0; axis < 3; ++axis) {
					if (!evaluate_number(
							arguments[static_cast<std::size_t>(axis + 4)],
							"Plant crown dimensions", evaluator,
							&dimensions[axis])) return result;
				}
				switch (kind) {
				case CrownVolumeKind::Ellipsoid:
					crown_volume = CrownVolumeSpecification::ellipsoid(
						origin, dimensions);
					break;
				case CrownVolumeKind::Cone:
					crown_volume = CrownVolumeSpecification::cone(
						origin, glm::vec2(dimensions.x, dimensions.z),
						dimensions.y);
					break;
				case CrownVolumeKind::InverseCone:
					crown_volume = CrownVolumeSpecification::inverseCone(
						origin, glm::vec2(dimensions.x, dimensions.z),
						dimensions.y);
					break;
				case CrownVolumeKind::Cylinder:
					crown_volume = CrownVolumeSpecification::cylinder(
						origin, glm::vec2(dimensions.x, dimensions.z),
						dimensions.y);
					break;
				case CrownVolumeKind::Dome:
					crown_volume = CrownVolumeSpecification::dome(
						origin, dimensions);
					break;
				case CrownVolumeKind::Lobed: {
					int lobe_count = 0;
					float lobe_amplitude = 0.0f;
					if (!evaluate_integer(
							arguments[7], "Plant crown lobe count", evaluator,
							&lobe_count, &result.diagnostic) ||
					    !evaluate_number(
							arguments[8], "Plant crown lobe amplitude", evaluator,
							&lobe_amplitude)) return result;
					crown_volume = CrownVolumeSpecification::lobed(
						origin, dimensions, lobe_count, lobe_amplitude);
					break;
				}
				case CrownVolumeKind::Sphere:
				case CrownVolumeKind::CustomSampled:
					break;
				}
			}
		}
		else if (option.optionName() == "crownCustom") {
			if (crown_volume.has_value()) {
				result.diagnostic =
					"Plant accepts either crown(...) or crownCustom(...), not both.";
				return result;
			}
			custom_crown_declared = true;
			if (!evaluate_number(
					arguments[0], "Plant custom crown sample radius", evaluator,
					&custom_crown_sample_radius)) return result;
		}
		else if (option.optionName() == "crownSample") {
			glm::vec3 sample(0.0f);
			for (int axis = 0; axis < 3; ++axis) {
				if (!evaluate_number(
						arguments[static_cast<std::size_t>(axis)],
						"Plant custom crown sample", evaluator, &sample[axis])) {
					return result;
				}
			}
			custom_crown_samples.push_back(sample);
		}
		else if (option.optionName() == "spaceColonization") {
			space_colonization_declared = true;
			if (!evaluate_number(
					arguments[0], "Plant colonization influence radius", evaluator,
					&colonization_influence_radius) ||
			    !evaluate_number(
					arguments[1], "Plant colonization kill radius", evaluator,
					&colonization_kill_radius) ||
			    !evaluate_number(
					arguments[2], "Plant colonization step length", evaluator,
					&colonization_step_length) ||
			    !evaluate_integer(
					arguments[3], "Plant colonization maximum iterations", evaluator,
					&colonization_maximum_iterations, &result.diagnostic) ||
			    !evaluate_integer(
					arguments[4], "Plant colonization attraction count", evaluator,
					&colonization_attraction_point_count, &result.diagnostic) ||
			    !evaluate_number(
					arguments[5], "Plant colonization radius decay", evaluator,
					&colonization_radius_decay) ||
			    !evaluate_number(
					arguments[6], "Plant colonization minimum radius", evaluator,
					&colonization_minimum_radius) ||
			    !evaluate_number(
					arguments[7], "Plant colonization radius gamma", evaluator,
					&colonization_radius_gamma)) return result;
		}
		else if (option.optionName() == "crownObstacle") {
			bool bounds_succeeded = false;
			const AxisAlignedBounds bounds = evaluated_bounds(
				arguments, 1u, "Plant crown obstacle bound", evaluator,
				&bounds_succeeded);
			if (!bounds_succeeded) return result;
			float clearance = 0.0f;
			if (!evaluate_number(
					arguments[7], "Plant crown obstacle clearance", evaluator,
					&clearance)) return result;
			crown_obstacles.emplace_back(
				arguments[0].sourceText(), bounds, clearance);
		}
		else if (option.optionName() == "floweringAge") {
			if (!evaluate_number(arguments[0], "Plant flowering age", evaluator,
			                     &flowering_age)) return result;
		}
		else if (option.optionName() == "matureAge") {
			if (!evaluate_number(arguments[0], "Plant mature age", evaluator,
			                     &mature_age)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &detail_level, &result.diagnostic)) return result;
		}
		else {
			result.diagnostic = "Option '" + option.optionName() +
			                    "' is not supported by Plant.";
			return result;
		}
	}

	if (!age_was_explicit) age = mature_age;
	if (!custom_crown_declared && !custom_crown_samples.empty()) {
		result.diagnostic =
			"Plant crownSample(...) requires crownCustom(sampleRadius).";
		return result;
	}
	if (custom_crown_declared) {
		crown_volume = CrownVolumeSpecification::customSampled(
			std::move(custom_crown_samples), custom_crown_sample_radius);
	}
	PlantTopologyGenerationSpecification topology_generation =
		PlantTopologyGenerationSpecification::createRuleBranching();
	const bool l_system_options_declared = l_system_axiom_declared ||
		l_system_parameters_declared || !l_system_rules.empty();
	if (topology_generation_method == PlantTopologyGenerationMethod::LSystem) {
		if (!l_system_axiom_declared || !l_system_parameters_declared ||
		    l_system_rules.empty()) {
			result.diagnostic =
				"Plant LSystem generation requires lAxiom(...), at least one lRule(...), and lSystem(...).";
			return result;
		}
		if (l_system_iteration_count < 0) {
			result.diagnostic = "Plant L-system iterations must be non-negative.";
			return result;
		}
		if (crown_volume.has_value() || space_colonization_declared ||
		    !crown_obstacles.empty()) {
			result.diagnostic =
				"Plant crown and colonization options are not valid with generation(LSystem).";
			return result;
		}
		topology_generation = PlantTopologyGenerationSpecification::createLSystem(
			PlantLSystemSpecification(
				std::move(l_system_axiom), std::move(l_system_rules),
				static_cast<std::size_t>(l_system_iteration_count),
				l_system_turn_angle_degrees, l_system_step_length,
				l_system_base_radius, l_system_terminal_radius,
				l_system_radius_gamma));
	}
	else if (topology_generation_method ==
	    PlantTopologyGenerationMethod::SpaceColonization) {
		if (l_system_options_declared) {
			result.diagnostic =
				"Plant L-system options require generation(LSystem).";
			return result;
		}
		if (!crown_volume.has_value() || !space_colonization_declared) {
			result.diagnostic =
				"Plant SpaceColonization generation requires crown(...) or crownCustom(...), plus spaceColonization(...).";
			return result;
		}
		glm::vec3 weighted_tropism(0.0f);
		for (const TropismInfluence &influence : tropism_influences) {
			if (influence.weight() <= 0.0f ||
			    glm::length(influence.direction()) <= 1.0e-6f) continue;
			weighted_tropism += influence.weight() *
				glm::normalize(influence.direction());
		}
		const float colonization_tropism_weight = glm::length(weighted_tropism);
		const glm::vec3 colonization_tropism_direction =
			colonization_tropism_weight > 1.0e-6f
				? weighted_tropism / colonization_tropism_weight
				: glm::vec3(0.0f);
		topology_generation =
			PlantTopologyGenerationSpecification::createSpaceColonization(
				PlantSpaceColonizationSpecification(
					std::move(*crown_volume),
					SpaceColonizationSpecification(
						colonization_influence_radius,
						colonization_kill_radius,
						colonization_step_length,
						static_cast<std::size_t>(
							std::max(0, colonization_maximum_iterations)),
						static_cast<std::size_t>(
							std::max(0, colonization_attraction_point_count)),
						colonization_radius_decay,
						colonization_minimum_radius,
						colonization_radius_gamma,
						colonization_tropism_direction,
						colonization_tropism_weight),
					std::move(crown_obstacles)));
	}
	else {
		if (l_system_options_declared) {
			result.diagnostic =
				"Plant L-system options require generation(LSystem).";
			return result;
		}
		if (crown_volume.has_value() || space_colonization_declared ||
		    !crown_obstacles.empty()) {
			result.diagnostic =
				"Plant crown and colonization options require generation(SpaceColonization).";
			return result;
		}
	}
	PlantShapeSpecificationCandidate candidate;
	candidate.species.emplace(
		catalog_species->identifier(), architecture,
		PlantBranchingSpecification(
			primary_axis_length, base_radius, tip_radius,
			primary_internode_count, maximum_branch_order,
			lateral_branches_per_node, branch_every_n_internodes,
			branch_angle_degrees, azimuth_divergence_degrees,
			branch_length_fraction, branch_length_falloff,
			continuation_radius_ratio, radius_conservation_gamma,
			direction_variation_degrees, length_variation_fraction,
			upward_tropism_weight),
		PlantLeafSpecification(
			leaf_profile, leaf_length, leaf_width, leaf_camber,
			leaf_twist_degrees, leaf_thickness, leaf_minimum_branch_order,
			PlantPetioleSpecification(
				petiole_length, petiole_base_radius, petiole_tip_radius),
			compound_leaf_enabled
				? PlantCompoundLeafSpecification(
					leaflet_node_count, rachis_length, rachis_base_radius,
					rachis_tip_radius, compound_leaf_pattern,
					compound_leaf_divergence, compound_leaf_up_bias,
					leaflet_scale)
				: PlantCompoundLeafSpecification(),
			PhyllotaxisSpecification(
				phyllotaxis_mode, phyllotaxis_divergence,
				phyllotaxis_internode_length, phyllotaxis_organs_per_node,
				phyllotaxis_phase, phyllotaxis_radial_offset,
				phyllotaxis_up_bias)),
		flowering_age, mature_age,
		flower_enabled
			? PlantFlowerSpecification(
				petal_profile, petal_length, petal_width, petal_curvature,
				petal_camber, petal_twist_degrees, petal_thickness,
					petal_width_power,
					WhorlSpecification(
						whorl_count, whorl_radius, whorl_phase, whorl_tilt),
					flower_head, inflorescence)
				: PlantFlowerSpecification(),
			std::move(organ_arrays),
		fruit_enabled
			? PlantFruitSpecification(
				fruit_height, fruit_radius, fruit_shoulder, fruit_fullness,
				fruit_radial_segments, fruit_profile_segments)
			: PlantFruitSpecification());
	candidate.age = age;
	candidate.development_state = development_state;
	candidate.deterministic_seed = deterministic_seed;
	candidate.topology_generation = std::move(topology_generation);
	candidate.growth = std::move(growth);
	candidate.tropism_influences = std::move(tropism_influences);
	candidate.detail_level = detail_level;
	result.specification = PlantShapeSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_vine(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	const PlantSpeciesSpecification *selected_species =
		PlantSpeciesCatalog().find("IvyVine");
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		if (option.optionName() != "species") continue;
		selected_species = PlantSpeciesCatalog().find(
			option.arguments()[0].sourceText());
		if (selected_species == nullptr) {
			result.diagnostic = "Unknown vine species '" +
			                    option.arguments()[0].sourceText() + "'.";
			return result;
		}
	}
	if (selected_species == nullptr) {
		result.diagnostic = "The IvyVine species data record is unavailable.";
		return result;
	}

	VineShapeSpecificationCandidate candidate;
	candidate.species = *selected_species;
	candidate.initial_radius = selected_species->branching().baseRadius();
	candidate.minimum_radius = selected_species->branching().tipRadius();
	candidate.radius_conservation_exponent =
		selected_species->branching().radiusConservationGamma();
	std::set<std::string> seen;

	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const bool repeatable = option.optionName() == "obstacle";
		if (!repeatable &&
		    !claim_single_option(option, &seen, &result.diagnostic)) {
			return result;
		}
		const std::vector<GeometryExpression> &arguments = option.arguments();
		if (option.optionName() == "species") {
			continue;
		}
		if (option.optionName() == "start" ||
		    option.optionName() == "direction" ||
		    option.optionName() == "preferred") {
			glm::vec3 value(0.0f);
			for (int axis = 0; axis < 3; ++axis) {
				if (!evaluate_number(
						arguments[static_cast<std::size_t>(axis)],
						"Vine " + option.optionName(), evaluator, &value[axis])) {
					return result;
				}
			}
			if (option.optionName() == "start") candidate.start_position = value;
			else if (option.optionName() == "direction") {
				candidate.initial_direction = value;
			}
			else candidate.preferred_direction = value;
		}
		else if (option.optionName() == "radius") {
			if (!evaluate_number(
					arguments[0], "Vine initial radius", evaluator,
					&candidate.initial_radius)) return result;
		}
		else if (option.optionName() == "mode") {
			if (!parse_vine_growth_mode(
					arguments[0], &candidate.growth_mode, &result.diagnostic)) {
				return result;
			}
		}
		else if (option.optionName() == "collision") {
			if (!parse_vegetation_collision_behavior(
					arguments[0], &candidate.collision_behavior,
					&result.diagnostic)) return result;
		}
		else if (option.optionName() == "attachment") {
			if (!parse_surface_attachment_mode(
					arguments[0], &candidate.attachment_mode,
					&result.diagnostic)) return result;
		}
		else if (option.optionName() == "step") {
			if (!evaluate_number(
					arguments[0], "Vine step length", evaluator,
					&candidate.step_length)) return result;
		}
		else if (option.optionName() == "segments") {
			int segments = 0;
			if (!evaluate_integer(
					arguments[0], "Vine segment count", evaluator, &segments,
					&result.diagnostic)) return result;
			if (segments < 0) {
				result.diagnostic = "Vine segment count must be non-negative.";
				return result;
			}
			candidate.maximum_segments = static_cast<std::size_t>(segments);
		}
		else if (option.optionName() == "seekDistance") {
			if (!evaluate_number(
					arguments[0], "Vine seek distance", evaluator,
					&candidate.maximum_seek_distance)) return result;
		}
		else if (option.optionName() == "attachDistance") {
			if (!evaluate_number(
					arguments[0], "Vine attachment distance", evaluator,
					&candidate.attachment_distance)) return result;
		}
		else if (option.optionName() == "tolerance") {
			if (!evaluate_number(
					arguments[0], "Vine attachment tolerance", evaluator,
					&candidate.attachment_tolerance)) return result;
		}
		else if (option.optionName() == "radiusDecay") {
			if (!evaluate_number(
					arguments[0], "Vine radius decay", evaluator,
					&candidate.radius_decay)) return result;
		}
		else if (option.optionName() == "minimumRadius") {
			if (!evaluate_number(
					arguments[0], "Vine minimum radius", evaluator,
					&candidate.minimum_radius)) return result;
		}
		else if (option.optionName() == "gamma") {
			if (!evaluate_number(
					arguments[0], "Vine radius conservation exponent", evaluator,
					&candidate.radius_conservation_exponent)) return result;
		}
		else if (option.optionName() == "target") {
			bool bounds_succeeded = false;
			const AxisAlignedBounds bounds = evaluated_bounds(
				arguments, 1u, "Vine target bound", evaluator,
				&bounds_succeeded);
			if (!bounds_succeeded) return result;
			candidate.target = VegetationSurfaceTarget(
				arguments[0].sourceText(),
				VegetationSurfaceGeometryKind::AxisAlignedBox, bounds);
		}
		else if (option.optionName() == "obstacle") {
			bool bounds_succeeded = false;
			const AxisAlignedBounds bounds = evaluated_bounds(
				arguments, 1u, "Vine obstacle bound", evaluator,
				&bounds_succeeded);
			if (!bounds_succeeded) return result;
			float clearance = 0.0f;
			if (!evaluate_number(
					arguments[7], "Vine obstacle clearance", evaluator,
					&clearance)) return result;
			candidate.obstacles.emplace_back(
				arguments[0].sourceText(), bounds, clearance);
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level,
					&result.diagnostic)) return result;
		}
		else {
			result.diagnostic = "Option '" + option.optionName() +
			                    "' is not supported by Vine.";
			return result;
		}
	}
	result.specification = VineShapeSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_scatter_region(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	const PlantSpeciesSpecification *selected_species =
		PlantSpeciesCatalog().find("GrassClump");
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		if (option.optionName() != "species") continue;
		selected_species = PlantSpeciesCatalog().find(
			option.arguments()[0].sourceText());
		if (selected_species == nullptr) {
			result.diagnostic = "Unknown ScatterRegion species '" +
			                    option.arguments()[0].sourceText() + "'.";
			return result;
		}
	}
	if (selected_species == nullptr) {
		result.diagnostic = "The GrassClump species data record is unavailable.";
		return result;
	}

	ScatterRegionShapeSpecificationCandidate candidate;
	candidate.species = *selected_species;
	std::set<std::string> seen;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const bool repeatable = option.optionName() == "obstacle";
		if (!repeatable &&
		    !claim_single_option(option, &seen, &result.diagnostic)) {
			return result;
		}
		const std::vector<GeometryExpression> &arguments = option.arguments();
		if (option.optionName() == "species") {
			continue;
		}
		if (option.optionName() == "region") {
			candidate.region_identifier = arguments[0].sourceText();
		}
		else if (option.optionName() == "seed") {
			int seed = 0;
			if (!evaluate_integer(
					arguments[0], "ScatterRegion seed", evaluator, &seed,
					&result.diagnostic)) return result;
			if (seed < 0) {
				result.diagnostic = "ScatterRegion seed must be non-negative.";
				return result;
			}
			candidate.deterministic_seed = static_cast<std::uint64_t>(seed);
		}
		else if (option.optionName() == "surface") {
			bool bounds_succeeded = false;
			const AxisAlignedBounds bounds = evaluated_bounds(
				arguments, 1u, "ScatterRegion surface bound", evaluator,
				&bounds_succeeded);
			if (!bounds_succeeded) return result;
			candidate.surface = VegetationSurfaceTarget(
				arguments[0].sourceText(),
				VegetationSurfaceGeometryKind::AxisAlignedBox, bounds);
		}
		else if (option.optionName() == "face") {
			if (!parse_scatter_surface_face(
					arguments[0], &candidate.surface_face,
					&result.diagnostic)) return result;
		}
		else if (option.optionName() == "density") {
			if (!evaluate_number(
					arguments[0], "ScatterRegion density", evaluator,
					&candidate.density)) return result;
		}
		else if (option.optionName() == "separation") {
			if (!evaluate_number(
					arguments[0], "ScatterRegion minimum separation", evaluator,
					&candidate.minimum_distance)) return result;
		}
		else if (option.optionName() == "scale") {
			if (!evaluate_number(
					arguments[0], "ScatterRegion minimum scale", evaluator,
					&candidate.scale_range.x) ||
			    !evaluate_number(
					arguments[1], "ScatterRegion maximum scale", evaluator,
					&candidate.scale_range.y)) return result;
		}
		else if (option.optionName() == "orientation") {
			if (!parse_scatter_orientation_mode(
					arguments[0], &candidate.orientation_mode,
					&result.diagnostic)) return result;
		}
		else if (option.optionName() == "collisionRadius") {
			if (!evaluate_number(
					arguments[0], "ScatterRegion collision radius", evaluator,
					&candidate.collision_radius)) return result;
		}
		else if (option.optionName() == "layer") {
			if (!parse_collision_layer(
					arguments[0], &candidate.placement_layer,
					&result.diagnostic)) return result;
		}
		else if (option.optionName() == "mask") {
			candidate.collision_mask = CollisionLayerMask();
			for (const GeometryExpression &argument : arguments) {
				CollisionLayer layer = CollisionLayer::Temporary;
				if (!parse_collision_layer(
						argument, &layer, &result.diagnostic)) return result;
				candidate.collision_mask.add(layer);
			}
		}
		else if (option.optionName() == "obstacle") {
			bool bounds_succeeded = false;
			const AxisAlignedBounds bounds = evaluated_bounds(
				arguments, 1u, "ScatterRegion obstacle bound", evaluator,
				&bounds_succeeded);
			if (!bounds_succeeded) return result;
			CollisionLayer layer = CollisionLayer::Temporary;
			if (!parse_collision_layer(
					arguments[7], &layer, &result.diagnostic)) return result;
			candidate.obstacles.emplace_back(
				arguments[0].sourceText(), bounds, layer);
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level,
					&result.diagnostic)) return result;
		}
		else {
			result.diagnostic = "Option '" + option.optionName() +
			                    "' is not supported by ScatterRegion.";
			return result;
		}
	}
	result.specification = ScatterRegionShapeSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

bool evaluate_axial_transform(
	const AxialProfileTransformSyntax &syntax,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator,
	AxialProfileLevelCandidate *candidate)
{
	return evaluate_number(syntax.centerX(), "AxialProfile center X", evaluator,
	                       &candidate->center.x) &&
	       evaluate_number(syntax.centerZ(), "AxialProfile center Z", evaluator,
	                       &candidate->center.y) &&
	       evaluate_number(syntax.scaleX(), "AxialProfile scale X", evaluator,
	                       &candidate->scale.x) &&
	       evaluate_number(syntax.scaleZ(), "AxialProfile scale Z", evaluator,
	                       &candidate->scale.y) &&
	       evaluate_number(syntax.rotation(), "AxialProfile rotation", evaluator,
	                       &candidate->rotation_degrees);
}

ShapeSpecificationEvaluationResult evaluate_axial_profile(
	const AxialProfileDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	if (descriptor.axisName() != "y") {
		result.diagnostic = "AxialProfilev1 supports only axis(y).";
		return result;
	}

	AxialProfileSpecificationCandidate candidate;
	for (const AxialProfilePolygonSyntax &profile_syntax : descriptor.profiles()) {
		AxialProfilePolygonCandidate profile_candidate;
		profile_candidate.name = profile_syntax.name();
		const std::vector<GeometryExpression> &coordinates =
			profile_syntax.coordinates();
		for (std::size_t coordinate_index = 0;
		     coordinate_index < coordinates.size();
		     coordinate_index += 2) {
			glm::vec2 vertex(0.0f);
			if (!evaluate_number(coordinates[coordinate_index],
			                     "AxialProfile polygon X", evaluator, &vertex.x) ||
			    !evaluate_number(coordinates[coordinate_index + 1],
			                     "AxialProfile polygon Z", evaluator, &vertex.y)) {
				return result;
			}
			profile_candidate.vertices.push_back(vertex);
		}
		candidate.profiles.push_back(std::move(profile_candidate));
	}

	for (const AxialProfileLevelSyntax &level_syntax : descriptor.levels()) {
		AxialProfileLevelCandidate level_candidate;
		level_candidate.transition = level_syntax.transition();
		if (level_syntax.transition() == AxialTransitionKind::Hold) {
			if (candidate.levels.empty()) {
				result.diagnostic = "AxialProfile hold() requires a current section.";
				return result;
			}
			level_candidate = candidate.levels.back();
			level_candidate.transition = AxialTransitionKind::Hold;
			if (!evaluate_number(level_syntax.axialPosition(),
			                     "AxialProfile hold position", evaluator,
			                     &level_candidate.axial_position)) {
				return result;
			}
			candidate.levels.push_back(std::move(level_candidate));
			continue;
		}

		level_candidate.profile_name = level_syntax.profileName();
		if (level_syntax.transition() == AxialTransitionKind::Step) {
			if (candidate.levels.empty()) {
				result.diagnostic = "AxialProfile step() requires a current section.";
				return result;
			}
			level_candidate.axial_position = candidate.levels.back().axial_position;
		} else if (!evaluate_number(level_syntax.axialPosition(),
		                           "AxialProfile axial position", evaluator,
		                           &level_candidate.axial_position)) {
			return result;
		}
		if (!evaluate_axial_transform(
			    level_syntax.transform(), evaluator, &level_candidate)) {
			return result;
		}
		candidate.levels.push_back(std::move(level_candidate));
	}

	if (descriptor.capName() == "all") {
		candidate.cap_policy = AxialProfileCapPolicy::createAll();
	} else if (descriptor.capName() == "none") {
		candidate.cap_policy = AxialProfileCapPolicy::createNone();
	} else if (descriptor.capName() == "bottom") {
		candidate.cap_policy = AxialProfileCapPolicy::createBottom();
	} else if (descriptor.capName() == "top") {
		candidate.cap_policy = AxialProfileCapPolicy::createTop();
	} else {
		result.diagnostic = "AxialProfile cap() accepts none, all, bottom, or top.";
		return result;
	}

	result.specification = AxialProfileSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

std::shared_ptr<const Profile2D> evaluate_profile_constructor(
	const std::string &constructor_name,
	const std::vector<GeometryExpression> &arguments,
	const std::string &purpose,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator,
	std::string *diagnostic)
{
	std::vector<float> values(arguments.size(), 0.0f);
	for (std::size_t index = 0; index < arguments.size(); ++index) {
		if (!evaluate_number(arguments[index], purpose, evaluator, &values[index])) {
			return {};
		}
	}

	const bool rectangle_arity = constructor_name == "Rect" && values.size() == 2u;
	const bool rounded_rectangle_arity = constructor_name == "RoundedRect" &&
		(values.size() == 3u || values.size() == 4u);
	const bool circle_arity = constructor_name == "Circle" &&
		(values.size() == 1u || values.size() == 2u);
	const bool ellipse_arity = constructor_name == "Ellipse" &&
		(values.size() == 2u || values.size() == 3u);
	const bool chamfer_rectangle_arity = constructor_name == "ChamferRect" &&
		values.size() == 3u;
	const bool thin_wall_box_arity =
		(constructor_name == "ThinWallBox" ||
		 constructor_name == "ThinWallChannel") && values.size() == 4u;
	const bool thin_wall_hat_arity = constructor_name == "ThinWallHat" &&
		values.size() == 5u;
	const bool thin_wall_multi_cell_arity =
		constructor_name == "ThinWallMultiCell" && values.size() == 5u;
	const bool polygon_arity = constructor_name == "Polygon" &&
		values.size() >= 6u && values.size() % 2u == 0u;
	if (!rectangle_arity && !rounded_rectangle_arity && !circle_arity &&
	    !ellipse_arity && !chamfer_rectangle_arity && !polygon_arity &&
	    !thin_wall_box_arity && !thin_wall_hat_arity &&
	    !thin_wall_multi_cell_arity) {
		if (diagnostic != nullptr) {
			*diagnostic = "Profile constructor '" + constructor_name +
			              "' has an unsupported argument count.";
		}
		return {};
	}

	Profile2DFactory profile_factory;
	if (constructor_name == "Rect") {
		return profile_factory.createRectangle(values[0], values[1], diagnostic);
	}
	if (constructor_name == "RoundedRect") {
		int segments = 4;
		if (values.size() == 4u) {
			const float rounded = std::round(values[3]);
			if (std::fabs(values[3] - rounded) > 1.0e-5f || rounded < 1.0f) {
				if (diagnostic != nullptr) {
					*diagnostic = "RoundedRect segment count must be a positive integer.";
				}
				return {};
			}
			segments = static_cast<int>(rounded);
		}
		return profile_factory.createRoundedRectangle(
			values[0], values[1], values[2], segments, diagnostic);
	}
	if (constructor_name == "Circle") {
		int segments = 32;
		if (values.size() == 2u) {
			const float rounded = std::round(values[1]);
			if (std::fabs(values[1] - rounded) > 1.0e-5f || rounded < 3.0f) {
				if (diagnostic != nullptr) {
					*diagnostic = "Circle segment count must be an integer of at least three.";
				}
				return {};
			}
			segments = static_cast<int>(rounded);
		}
		return profile_factory.createCircle(values[0], segments, diagnostic);
	}
	if (constructor_name == "Ellipse") {
		int segments = 32;
		if (values.size() == 3u) {
			const float rounded = std::round(values[2]);
			if (std::fabs(values[2] - rounded) > 1.0e-5f || rounded < 3.0f) {
				if (diagnostic != nullptr) {
					*diagnostic = "Ellipse segment count must be an integer of at least three.";
				}
				return {};
			}
			segments = static_cast<int>(rounded);
		}
		return profile_factory.createEllipse(
			values[0], values[1], segments, diagnostic);
	}
	if (constructor_name == "ChamferRect") {
		return profile_factory.createChamferRectangle(
			values[0], values[1], values[2], diagnostic);
	}
	if (constructor_name == "ThinWallBox" ||
	    constructor_name == "ThinWallChannel" ||
	    constructor_name == "ThinWallHat" ||
	    constructor_name == "ThinWallMultiCell") {
		ThinWallProfileFactory thin_wall_factory;
		std::shared_ptr<const ThinWallProfile2D> thin_wall_profile;
		if (constructor_name == "ThinWallBox") {
			thin_wall_profile = thin_wall_factory.createClosedBox(
				values[0], values[1], values[2], values[3], diagnostic);
		}
		else if (constructor_name == "ThinWallChannel") {
			thin_wall_profile = thin_wall_factory.createOpenChannel(
				values[0], values[1], values[2], values[3], diagnostic);
		}
		else if (constructor_name == "ThinWallHat") {
			thin_wall_profile = thin_wall_factory.createHatSection(
				values[0], values[1], values[2], values[3], values[4], diagnostic);
		}
		else {
			const float rounded_cell_count = std::round(values[4]);
			if (std::fabs(values[4] - rounded_cell_count) > 1.0e-5f ||
			    rounded_cell_count < 2.0f) {
				if (diagnostic != nullptr) {
					*diagnostic = "ThinWallMultiCell cell count must be an integer of at least two.";
				}
				return {};
			}
			thin_wall_profile = thin_wall_factory.createMultiCellBox(
				values[0], values[1], values[2], values[3],
				static_cast<std::size_t>(rounded_cell_count), diagnostic);
		}
		if (!thin_wall_profile) return {};
		return std::make_shared<const Profile2D>(thin_wall_profile->profile());
	}

	std::vector<glm::vec2> points;
	points.reserve(values.size() / 2u);
	for (std::size_t index = 0; index < values.size(); index += 2u) {
		points.emplace_back(values[index], values[index + 1u]);
	}
	return profile_factory.createPolygon(std::move(points), {}, diagnostic);
}

void copy_profile_to_candidate(
	const Profile2D &profile,
	Profile2DCandidate *candidate)
{
	candidate->outer_loop = profile.outerLoop().points();
	candidate->inner_loops.clear();
	for (const ProfileLoop2D &inner_loop : profile.innerLoops()) {
		candidate->inner_loops.push_back(inner_loop.points());
	}
}

bool evaluate_path3d(
	const std::vector<GeometryExpression> &arguments,
	const std::string &purpose,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator,
	std::vector<glm::vec3> *path,
	std::string *diagnostic)
{
	if (arguments.size() < 6u || arguments.size() % 3u != 0u) {
		if (diagnostic != nullptr) {
			*diagnostic = purpose + " requires at least two x y z point groups.";
		}
		return false;
	}
	path->clear();
	path->reserve(arguments.size() / 3u);
	for (std::size_t index = 0; index < arguments.size(); index += 3u) {
		glm::vec3 point(0.0f);
		if (!evaluate_number(arguments[index], purpose + " X", evaluator, &point.x) ||
		    !evaluate_number(arguments[index + 1u], purpose + " Y", evaluator, &point.y) ||
		    !evaluate_number(arguments[index + 2u], purpose + " Z", evaluator, &point.z)) {
			return false;
		}
		path->push_back(point);
	}
	return true;
}

std::optional<Curve3D> evaluate_curve3d(
	const std::vector<GeometryExpression> &arguments,
	const std::string &purpose,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator,
	std::string *diagnostic)
{
	if (arguments.empty()) {
		if (diagnostic != nullptr) *diagnostic = purpose + " requires a curve family.";
		return std::nullopt;
	}
	Curve3DType curve_type = Curve3DType::Polyline;
	const std::string family = lowercase_copy(arguments.front().sourceText());
	if (family == "line") curve_type = Curve3DType::Line;
	else if (family == "polyline") curve_type = Curve3DType::Polyline;
	else if (family == "bezier" || family == "beziercubic") {
		curve_type = Curve3DType::Bezier;
	}
	else if (family == "catmullrom") curve_type = Curve3DType::CatmullRom;
	else {
		if (diagnostic != nullptr) {
			*diagnostic = purpose +
				" family must be line, polyline, bezier, or catmullRom.";
		}
		return std::nullopt;
	}
	std::vector<glm::vec3> control_points;
	const std::vector<GeometryExpression> coordinate_arguments(
		arguments.begin() + 1, arguments.end());
	if (!evaluate_path3d(
			coordinate_arguments, purpose, evaluator, &control_points, diagnostic)) {
		return std::nullopt;
	}
	Curve3D curve(curve_type, std::move(control_points), purpose);
	if (!curve.isValid(diagnostic)) return std::nullopt;
	return curve;
}

bool evaluate_path2d(
	const std::vector<GeometryExpression> &arguments,
	const std::string &purpose,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator,
	std::vector<glm::vec2> *path,
	std::string *diagnostic)
{
	if (arguments.size() < 4u || arguments.size() % 2u != 0u) {
		if (diagnostic != nullptr) {
			*diagnostic = purpose + " requires at least two x y point groups.";
		}
		return false;
	}
	path->clear();
	path->reserve(arguments.size() / 2u);
	for (std::size_t index = 0; index < arguments.size(); index += 2u) {
		glm::vec2 point(0.0f);
		if (!evaluate_number(arguments[index], purpose + " X", evaluator, &point.x) ||
		    !evaluate_number(arguments[index + 1u], purpose + " Y", evaluator, &point.y)) {
			return false;
		}
		path->push_back(point);
	}
	return true;
}

ShapeSpecificationEvaluationResult evaluate_extrude_profile(
	const ExtrudeProfileDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	const Profile2DConstructorSyntax &profile_syntax = descriptor.profile();
	const std::vector<GeometryExpression> &arguments = profile_syntax.arguments();
	const std::string &constructor_name = profile_syntax.constructorName();
	const std::shared_ptr<const Profile2D> profile = evaluate_profile_constructor(
		constructor_name, arguments, "Extrude profile argument", evaluator,
		&result.diagnostic);
	if (!profile) return result;

	ExtrudeProfileShapeSpecificationCandidate candidate;
	copy_profile_to_candidate(*profile, &candidate.profile);
	if (!evaluate_number(descriptor.depth(), "Extrude depth", evaluator, &candidate.depth)) {
		return result;
	}
	if (descriptor.capName() == "all") {
		candidate.cap_policy = ExtrudeProfileCapPolicy::createAll();
	}
	else if (descriptor.capName() == "none") {
		candidate.cap_policy = ExtrudeProfileCapPolicy::createNone();
	}
	else if (descriptor.capName() == "front") {
		candidate.cap_policy = ExtrudeProfileCapPolicy::createFront();
	}
	else {
		candidate.cap_policy = ExtrudeProfileCapPolicy::createBack();
	}
	result.specification = ExtrudeProfileSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_sweep_profile(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	SweepProfileShapeSpecificationCandidate candidate;
	bool profile_seen = false;
	bool path_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "profile") {
			const std::string constructor_name = arguments.front().sourceText();
			const std::vector<GeometryExpression> profile_arguments(
				arguments.begin() + 1, arguments.end());
			const auto profile = evaluate_profile_constructor(
				constructor_name, profile_arguments, "SweepProfile profile argument",
				evaluator, &result.diagnostic);
			if (!profile) return result;
			copy_profile_to_candidate(*profile, &candidate.profile);
			profile_seen = true;
		}
		else if (option.optionName() == "path") {
			if (!evaluate_path3d(arguments, "SweepProfile path", evaluator,
			                    &candidate.path_points, &result.diagnostic)) {
				return result;
			}
			path_seen = true;
		}
		else if (option.optionName() == "up") {
			if (!evaluate_number(arguments[0], "SweepProfile up X", evaluator,
			                     &candidate.up_hint.x) ||
			    !evaluate_number(arguments[1], "SweepProfile up Y", evaluator,
			                     &candidate.up_hint.y) ||
			    !evaluate_number(arguments[2], "SweepProfile up Z", evaluator,
			                     &candidate.up_hint.z)) return result;
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.cap_policy, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (!profile_seen || !path_seen) {
		result.diagnostic = "SweepProfile requires profile(...) and path(...).";
		return result;
	}
	result.specification = SweepProfileSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_variable_section_sweep(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	VariableSectionSweepShapeSpecificationCandidate candidate;
	struct StationTransformRecord
	{
		float normalized_path_position = 0.0f;
		glm::vec2 center{0.0f};
		glm::vec2 scale{1.0f};
		float rotation_degrees = 0.0f;
	};
	std::vector<StationTransformRecord> station_transforms;
	bool path_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "path") {
			if (!evaluate_path3d(arguments, "VariableSectionSweep path", evaluator,
			                    &candidate.path_points, &result.diagnostic)) return result;
			path_seen = true;
		}
		else if (option.optionName() == "station") {
			VariableSectionSweepStationCandidate station;
			if (!evaluate_number(
					arguments[0], "VariableSectionSweep station position", evaluator,
					&station.normalized_path_position)) return result;
			const std::string constructor_name = arguments[1].sourceText();
			const std::vector<GeometryExpression> profile_arguments(
				arguments.begin() + 2, arguments.end());
			const std::shared_ptr<const Profile2D> profile = evaluate_profile_constructor(
				constructor_name, profile_arguments,
				"VariableSectionSweep station profile argument", evaluator,
				&result.diagnostic);
			if (!profile) return result;
			copy_profile_to_candidate(*profile, &station.profile);
			candidate.stations.push_back(std::move(station));
		}
		else if (option.optionName() == "stationTransform") {
			StationTransformRecord transform;
			if (!evaluate_number(arguments[0], "VariableSectionSweep transform station", evaluator,
			                     &transform.normalized_path_position) ||
			    !evaluate_number(arguments[1], "VariableSectionSweep center X", evaluator,
			                     &transform.center.x) ||
			    !evaluate_number(arguments[2], "VariableSectionSweep center Y", evaluator,
			                     &transform.center.y) ||
			    !evaluate_number(arguments[3], "VariableSectionSweep scale X", evaluator,
			                     &transform.scale.x) ||
			    !evaluate_number(arguments[4], "VariableSectionSweep scale Y", evaluator,
			                     &transform.scale.y) ||
			    !evaluate_number(arguments[5], "VariableSectionSweep rotation", evaluator,
			                     &transform.rotation_degrees)) return result;
			station_transforms.push_back(transform);
		}
		else if (option.optionName() == "nominalSection") {
			const std::string constructor_name = arguments.front().sourceText();
			const std::vector<GeometryExpression> profile_arguments(
				arguments.begin() + 1, arguments.end());
			const std::shared_ptr<const Profile2D> profile = evaluate_profile_constructor(
				constructor_name, profile_arguments,
				"VariableSectionSweep nominal section profile argument", evaluator,
				&result.diagnostic);
			if (!profile) return result;
			Profile2DCandidate nominal_section_profile;
			copy_profile_to_candidate(*profile, &nominal_section_profile);
			candidate.nominal_section_profiles.push_back(
				std::move(nominal_section_profile));
		}
		else if (option.optionName() == "up") {
			if (!evaluate_number(arguments[0], "VariableSectionSweep up X", evaluator,
			                     &candidate.up_hint.x) ||
			    !evaluate_number(arguments[1], "VariableSectionSweep up Y", evaluator,
			                     &candidate.up_hint.y) ||
			    !evaluate_number(arguments[2], "VariableSectionSweep up Z", evaluator,
			                     &candidate.up_hint.z)) return result;
		}
		else if (option.optionName() == "frame") {
			const std::string frame_policy = lowercase_copy(arguments[0].sourceText());
			if (frame_policy == "rotationminimizing") {
				candidate.frame_policy = SweepFramePolicy::RotationMinimizing;
			}
			else if (frame_policy == "referencealigned") {
				candidate.frame_policy = SweepFramePolicy::ReferenceAligned;
			}
			else {
				result.diagnostic = "VariableSectionSweep frame must be rotationMinimizing or referenceAligned.";
				return result;
			}
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.cap_policy, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (!path_seen || candidate.stations.size() < 2u) {
		result.diagnostic = "VariableSectionSweep requires path(...) and at least two station(...) records.";
		return result;
	}
	for (const StationTransformRecord &transform : station_transforms) {
		auto station = std::find_if(
			candidate.stations.begin(), candidate.stations.end(),
			[&transform](const VariableSectionSweepStationCandidate &candidate_station) {
				return std::fabs(candidate_station.normalized_path_position -
				                 transform.normalized_path_position) <= 1.0e-5f;
			});
		if (station == candidate.stations.end()) {
			result.diagnostic = "VariableSectionSweep stationTransform references an undefined station position.";
			return result;
		}
		station->center = transform.center;
		station->scale = transform.scale;
		station->rotation_degrees = transform.rotation_degrees;
	}
	result.specification = VariableSectionSweepSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_sweep_disk(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	SweepDiskShapeSpecificationCandidate candidate;
	bool path_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "path") {
			if (!evaluate_path3d(arguments, "SweepDisk path", evaluator,
			                    &candidate.path_points, &result.diagnostic)) {
				return result;
			}
			path_seen = true;
		}
		else if (option.optionName() == "curve") {
			const std::string curve_type = lowercase_copy(arguments.front().sourceText());
			if (curve_type == "line") candidate.curve_type = Curve3DType::Line;
			else if (curve_type == "polyline") candidate.curve_type = Curve3DType::Polyline;
			else if (curve_type == "bezier" || curve_type == "beziercubic") {
				candidate.curve_type = Curve3DType::Bezier;
			}
			else if (curve_type == "catmullrom") {
				candidate.curve_type = Curve3DType::CatmullRom;
			}
			else {
				result.diagnostic = "SweepDisk curve family must be line, polyline, bezier, or catmullRom.";
				return result;
			}
			const std::vector<GeometryExpression> coordinate_arguments(
				arguments.begin() + 1, arguments.end());
			if (!evaluate_path3d(
					coordinate_arguments, "SweepDisk curve", evaluator,
					&candidate.curve_control_points, &result.diagnostic)) {
				return result;
			}
			candidate.curve_was_explicit = true;
			path_seen = true;
		}
		else if (option.optionName() == "up") {
			if (!evaluate_number(arguments[0], "SweepDisk up X", evaluator,
			                     &candidate.up_hint.x) ||
			    !evaluate_number(arguments[1], "SweepDisk up Y", evaluator,
			                     &candidate.up_hint.y) ||
			    !evaluate_number(arguments[2], "SweepDisk up Z", evaluator,
			                     &candidate.up_hint.z)) return result;
		}
		else if (option.optionName() == "radius") {
			if (!evaluate_number(arguments[0], "SweepDisk radius", evaluator,
			                     &candidate.radius)) return result;
			candidate.radius_was_explicit = true;
		}
		else if (option.optionName() == "radiusStart") {
			if (!evaluate_number(arguments[0], "SweepDisk start radius", evaluator,
			                     &candidate.radius_start)) return result;
			candidate.radius_start_was_explicit = true;
		}
		else if (option.optionName() == "radiusEnd") {
			if (!evaluate_number(arguments[0], "SweepDisk end radius", evaluator,
			                     &candidate.radius_end)) return result;
			candidate.radius_end_was_explicit = true;
		}
		else if (option.optionName() == "longitudinalSegments") {
			if (!evaluate_integer(
					arguments[0], "SweepDisk longitudinal segments", evaluator,
					&candidate.longitudinal_segments, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "segments" ||
		         option.optionName() == "radialSegments") {
			if (!evaluate_integer(arguments[0], "SweepDisk segments", evaluator,
			                      &candidate.circumferential_segments,
			                      &result.diagnostic)) return result;
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.cap_policy, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (!path_seen) {
		result.diagnostic = "SweepDisk requires path(...) or curve(...).";
		return result;
	}
	result.specification = SweepDiskSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_revolve(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	RevolveShapeSpecificationCandidate candidate;
	bool profile_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "profile") {
			const std::string constructor_name = arguments.front().sourceText();
			const std::vector<GeometryExpression> profile_arguments(
				arguments.begin() + 1, arguments.end());
			const auto profile = evaluate_profile_constructor(
				constructor_name, profile_arguments, "Revolve profile argument",
				evaluator, &result.diagnostic);
			if (!profile) return result;
			copy_profile_to_candidate(*profile, &candidate.radial_profile);
			profile_seen = true;
		}
		else if (option.optionName() == "angle") {
			if (!evaluate_number(arguments[0], "Revolve start angle", evaluator,
			                     &candidate.start_degrees) ||
			    !evaluate_number(arguments[1], "Revolve sweep angle", evaluator,
			                     &candidate.sweep_degrees)) return result;
		}
		else if (option.optionName() == "segments") {
			if (!evaluate_integer(arguments[0], "Revolve segments", evaluator,
			                      &candidate.angular_segments,
			                      &result.diagnostic)) return result;
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.angular_cap_policy,
					&result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (!profile_seen) {
		result.diagnostic = "Revolve requires profile(...).";
		return result;
	}
	result.specification = RevolveSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_loft(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	LoftShapeSpecificationCandidate candidate;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "section") {
			LoftSectionCandidate section;
			if (!evaluate_number(arguments[0], "Loft section position", evaluator,
			                     &section.axial_position)) return result;
			const std::string constructor_name = arguments[1].sourceText();
			const std::vector<GeometryExpression> profile_arguments(
				arguments.begin() + 2, arguments.end());
			const auto profile = evaluate_profile_constructor(
				constructor_name, profile_arguments, "Loft section profile argument",
				evaluator, &result.diagnostic);
			if (!profile) return result;
			copy_profile_to_candidate(*profile, &section.profile);
			candidate.sections.push_back(std::move(section));
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.cap_policy, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (candidate.sections.size() < 2u) {
		result.diagnostic = "Loft requires at least two section(...) records.";
		return result;
	}
	result.specification = LoftSpecificationValidator().validateLoft(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_surface_loft(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	LoftShapeSpecificationCandidate candidate;
	candidate.cap_policy = ExtrudeProfileCapPolicy::createNone();
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "section") {
			LoftSectionCandidate section;
			if (!evaluate_number(arguments[0], "SurfaceLoft section position", evaluator,
			                     &section.axial_position)) return result;
			const std::string constructor_name = arguments[1].sourceText();
			const std::vector<GeometryExpression> profile_arguments(
				arguments.begin() + 2, arguments.end());
			const auto profile = evaluate_profile_constructor(
				constructor_name, profile_arguments,
				"SurfaceLoft section profile argument", evaluator,
				&result.diagnostic);
			if (!profile) return result;
			copy_profile_to_candidate(*profile, &section.profile);
			candidate.sections.push_back(std::move(section));
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (candidate.sections.size() < 2u) {
		result.diagnostic = "SurfaceLoft requires at least two section(...) records.";
		return result;
	}
	result.specification = LoftSpecificationValidator().validateSurfaceLoft(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_curve_network_surface(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	CurveNetworkSurfaceShapeSpecificationCandidate candidate;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "uCurve" || option.optionName() == "vCurve") {
			const std::optional<Curve3D> curve = evaluate_curve3d(
				arguments,
				option.optionName() == "uCurve"
					? "CurveNetworkSurface constant-U curve"
					: "CurveNetworkSurface constant-V curve",
				evaluator,
				&result.diagnostic);
			if (!curve) return result;
			if (option.optionName() == "uCurve") {
				candidate.constant_u_curves.push_back(*curve);
			}
			else {
				candidate.constant_v_curves.push_back(*curve);
			}
		}
		else if (option.optionName() == "samplesU") {
			if (!evaluate_integer(arguments[0], "CurveNetworkSurface U samples",
			                      evaluator, &candidate.samples_u,
			                      &result.diagnostic)) return result;
		}
		else if (option.optionName() == "samplesV") {
			if (!evaluate_integer(arguments[0], "CurveNetworkSurface V samples",
			                      evaluator, &candidate.samples_v,
			                      &result.diagnostic)) return result;
		}
		else if (option.optionName() == "tolerance") {
			float tolerance = 0.0f;
			if (!evaluate_number(arguments[0], "CurveNetworkSurface tolerance",
			                     evaluator, &tolerance)) return result;
			candidate.intersection_tolerance = tolerance;
		}
		else if (option.optionName() == "method") {
			if (lowercase_copy(arguments[0].sourceText()) != "interpolatingpatchgrid") {
				result.diagnostic =
					"CurveNetworkSurface P0 method must be interpolatingPatchGrid.";
				return result;
			}
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level,
					&result.diagnostic)) return result;
		}
	}
	result.specification = CurveNetworkSurfaceSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_shell_loft(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	ShellLoftShapeSpecificationCandidate candidate;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "section") {
			ShellLoftSectionCandidate section;
			if (!evaluate_number(arguments[0], "ShellLoft section position", evaluator,
			                     &section.axial_position)) return result;
			int outer_point_count = 0;
			if (!evaluate_integer(arguments[1], "ShellLoft outer point count", evaluator,
			                      &outer_point_count, &result.diagnostic)) return result;
			const std::size_t outer_coordinate_count =
				outer_point_count > 0 ? static_cast<std::size_t>(outer_point_count) * 2u : 0u;
			const std::size_t coordinate_count = arguments.size() - 2u;
			if (outer_point_count < 3 || coordinate_count < outer_coordinate_count + 6u ||
			    coordinate_count % 2u != 0u) {
				result.diagnostic = "ShellLoft section requires an outer point count followed by outer and inner x y loops with at least three points each.";
				return result;
			}
			for (std::size_t coordinate = 0; coordinate < coordinate_count; coordinate += 2u) {
				glm::vec2 point(0.0f);
				if (!evaluate_number(arguments[coordinate + 2u], "ShellLoft section X", evaluator,
				                     &point.x) ||
				    !evaluate_number(arguments[coordinate + 3u], "ShellLoft section Y", evaluator,
				                     &point.y)) return result;
				if (coordinate < outer_coordinate_count) section.outer_loop.push_back(point);
				else section.inner_loop.push_back(point);
			}
			candidate.sections.push_back(std::move(section));
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.cap_policy, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (candidate.sections.size() < 2u) {
		result.diagnostic = "ShellLoft requires at least two section(...) records.";
		return result;
	}
	result.specification = LoftSpecificationValidator().validateShellLoft(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_folded_profile(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	FoldedProfileShapeSpecificationCandidate candidate;
	bool path_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "path") {
			if (!evaluate_path2d(arguments, "FoldedProfile path", evaluator,
			                    &candidate.fold_path, &result.diagnostic)) return result;
			path_seen = true;
		}
		else if (option.optionName() == "thickness") {
			if (!evaluate_number(arguments[0], "FoldedProfile thickness", evaluator,
			                     &candidate.thickness)) return result;
		}
		else if (option.optionName() == "depth") {
			if (!evaluate_number(arguments[0], "FoldedProfile depth", evaluator,
			                     &candidate.extrusion_depth)) return result;
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.cap_policy, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (!path_seen) {
		result.diagnostic = "FoldedProfile requires path(...).";
		return result;
	}
	result.specification = FoldedProfileSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_curved_panel(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	CurvedPanelShapeSpecificationCandidate candidate;
	bool width_seen = false;
	bool height_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "width") {
			if (!evaluate_number(arguments[0], "CurvedPanel width", evaluator,
			                     &candidate.width)) return result;
			width_seen = true;
		}
		else if (option.optionName() == "height") {
			if (!evaluate_number(arguments[0], "CurvedPanel height", evaluator,
			                     &candidate.height)) return result;
			height_seen = true;
		}
		else if (option.optionName() == "curvature") {
			if (!evaluate_number(arguments[0], "CurvedPanel horizontal curvature", evaluator,
			                     &candidate.horizontal_curvature)) return result;
			candidate.vertical_curvature = candidate.horizontal_curvature;
			if (arguments.size() == 2u &&
			    !evaluate_number(arguments[1], "CurvedPanel vertical curvature", evaluator,
			                     &candidate.vertical_curvature)) return result;
		}
		else if (option.optionName() == "thickness") {
			if (!evaluate_number(arguments[0], "CurvedPanel thickness", evaluator,
			                     &candidate.thickness)) return result;
		}
		else if (option.optionName() == "segments") {
			if (!evaluate_integer(arguments[0], "CurvedPanel horizontal segments", evaluator,
			                      &candidate.horizontal_segments,
			                      &result.diagnostic)) return result;
			candidate.vertical_segments = candidate.horizontal_segments;
			if (arguments.size() == 2u &&
			    !evaluate_integer(arguments[1], "CurvedPanel vertical segments", evaluator,
			                      &candidate.vertical_segments,
			                      &result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (!width_seen || !height_seen) {
		result.diagnostic = "CurvedPanel requires width(...) and height(...).";
		return result;
	}
	result.specification = CurvedPanelSpecificationValidator().validate(
		candidate, &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_formed_panel(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	FormedPanelShapeSpecificationCandidate candidate;
	bool thickness_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "section") {
			LoftSectionCandidate section;
			if (!evaluate_number(arguments[0], "FormedPanel section position", evaluator,
			                     &section.axial_position)) return result;
			const std::string constructor_name = arguments[1].sourceText();
			const std::vector<GeometryExpression> profile_arguments(
				arguments.begin() + 2, arguments.end());
			const auto profile = evaluate_profile_constructor(
				constructor_name, profile_arguments,
				"FormedPanel section profile argument", evaluator, &result.diagnostic);
			if (!profile) return result;
			copy_profile_to_candidate(*profile, &section.profile);
			candidate.sections.push_back(std::move(section));
		}
		else if (option.optionName() == "thickness") {
			if (!evaluate_number(arguments[0], "FormedPanel thickness", evaluator,
			                     &candidate.thickness)) return result;
			thickness_seen = true;
		}
		else if (option.optionName() == "side") {
			const std::string side = lowercase_copy(arguments[0].sourceText());
			if (side == "outward") candidate.offset_side = ShellOffsetSide::Outward;
			else if (side == "inward") candidate.offset_side = ShellOffsetSide::Inward;
			else if (side == "both") candidate.offset_side = ShellOffsetSide::Both;
			else {
				result.diagnostic = "FormedPanel side must be outward, inward, or both.";
				return result;
			}
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (candidate.sections.size() < 2u || !thickness_seen) {
		result.diagnostic = "FormedPanel requires at least two section(...) records and thickness(...).";
		return result;
	}
	result.specification = FormedPanelSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_hosted_opening(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	HostedOpeningShapeSpecificationCandidate candidate;
	bool host_seen = false;
	bool depth_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "host" || option.optionName() == "opening") {
			const std::string constructor_name = arguments[0].sourceText();
			const std::vector<GeometryExpression> profile_arguments(
				arguments.begin() + 1, arguments.end());
			const auto profile = evaluate_profile_constructor(
				constructor_name, profile_arguments,
				"HostedOpening profile argument", evaluator, &result.diagnostic);
			if (!profile) return result;
			if (!profile->innerLoops().empty()) {
				result.diagnostic = "HostedOpening host/opening constructors cannot contain nested holes.";
				return result;
			}
			if (option.optionName() == "host") {
				candidate.host_boundary = profile->outerLoop().points();
				host_seen = true;
			}
			else {
				candidate.opening_boundaries.push_back(profile->outerLoop().points());
			}
		}
		else if (option.optionName() == "depth") {
			if (!evaluate_number(arguments[0], "HostedOpening depth", evaluator,
			                     &candidate.depth)) return result;
			depth_seen = true;
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.cap_policy, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (!host_seen || !depth_seen || candidate.opening_boundaries.empty()) {
		result.diagnostic = "HostedOpening requires host(...), one or more opening(...), and depth(...).";
		return result;
	}
	result.specification = HostedOpeningSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_embossed_bead(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	EmbossedBeadShapeSpecificationCandidate candidate;
	bool path_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "path") {
			if (!evaluate_path3d(arguments, "EmbossedBead path", evaluator,
			                    &candidate.path_points, &result.diagnostic)) return result;
			path_seen = true;
		}
		else if (option.optionName() == "up") {
			if (!evaluate_number(arguments[0], "EmbossedBead up X", evaluator, &candidate.up_hint.x) ||
			    !evaluate_number(arguments[1], "EmbossedBead up Y", evaluator, &candidate.up_hint.y) ||
			    !evaluate_number(arguments[2], "EmbossedBead up Z", evaluator, &candidate.up_hint.z)) return result;
		}
		else if (option.optionName() == "width") {
			if (!evaluate_number(arguments[0], "EmbossedBead width", evaluator, &candidate.width)) return result;
		}
		else if (option.optionName() == "depth") {
			if (!evaluate_number(arguments[0], "EmbossedBead depth", evaluator, &candidate.depth)) return result;
		}
		else if (option.optionName() == "shoulder") {
			if (!evaluate_number(arguments[0], "EmbossedBead shoulder", evaluator, &candidate.shoulder_radius)) return result;
		}
		else if (option.optionName() == "side") {
			const std::string side = lowercase_copy(arguments[0].sourceText());
			if (side == "positive") candidate.side = EmbossedBeadSide::Positive;
			else if (side == "negative") candidate.side = EmbossedBeadSide::Negative;
			else {
				result.diagnostic = "EmbossedBead side must be positive or negative.";
				return result;
			}
		}
		else if (option.optionName() == "end") {
			const std::string end = lowercase_copy(arguments[0].sourceText());
			if (end == "closed") candidate.end_style = EmbossedBeadEndStyle::Closed;
			else if (end == "open") candidate.end_style = EmbossedBeadEndStyle::Open;
			else {
				result.diagnostic = "EmbossedBead end must be closed or open.";
				return result;
			}
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(arguments[0], &candidate.detail_level,
			                                 &result.diagnostic)) return result;
		}
	}
	if (!path_seen) {
		result.diagnostic = "EmbossedBead requires path(...).";
		return result;
	}
	result.specification = EmbossedBeadSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_edge_flange(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	EdgeFlangeShapeSpecificationCandidate candidate;
	bool path_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "path") {
			if (!evaluate_path3d(arguments, "EdgeFlange path", evaluator,
			                    &candidate.boundary_path, &result.diagnostic)) return result;
			path_seen = true;
		}
		else if (option.optionName() == "up") {
			if (!evaluate_number(arguments[0], "EdgeFlange up X", evaluator, &candidate.up_hint.x) ||
			    !evaluate_number(arguments[1], "EdgeFlange up Y", evaluator, &candidate.up_hint.y) ||
			    !evaluate_number(arguments[2], "EdgeFlange up Z", evaluator, &candidate.up_hint.z)) return result;
		}
		else if (option.optionName() == "width") {
			if (!evaluate_number(arguments[0], "EdgeFlange width", evaluator, &candidate.width)) return result;
		}
		else if (option.optionName() == "thickness") {
			if (!evaluate_number(arguments[0], "EdgeFlange thickness", evaluator, &candidate.thickness)) return result;
		}
		else if (option.optionName() == "angle") {
			if (!evaluate_number(arguments[0], "EdgeFlange angle", evaluator, &candidate.angle_degrees)) return result;
		}
		else if (option.optionName() == "bend") {
			if (!evaluate_number(arguments[0], "EdgeFlange bend", evaluator, &candidate.bend_radius)) return result;
		}
		else if (option.optionName() == "side") {
			const std::string side = lowercase_copy(arguments[0].sourceText());
			if (side == "positive") candidate.side = EdgeFlangeSide::Positive;
			else if (side == "negative") candidate.side = EdgeFlangeSide::Negative;
			else {
				result.diagnostic = "EdgeFlange side must be positive or negative.";
				return result;
			}
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(arguments[0], &candidate.detail_level,
			                                 &result.diagnostic)) return result;
		}
	}
	if (!path_seen) {
		result.diagnostic = "EdgeFlange requires path(...).";
		return result;
	}
	result.specification = EdgeFlangeSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_panel_cut(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	SweepProfileShapeSpecificationCandidate candidate;
	bool path_seen = false;
	bool gap_seen = false;
	float gap_width = 0.0f;
	float gap_depth = 0.0f;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "path") {
			if (!evaluate_path3d(arguments, "PanelCut path", evaluator,
			                    &candidate.path_points, &result.diagnostic)) return result;
			path_seen = true;
		}
		else if (option.optionName() == "gap") {
			if (!evaluate_number(arguments[0], "PanelCut gap width", evaluator,
			                     &gap_width)) return result;
			gap_depth = gap_width;
			if (arguments.size() == 2u &&
			    !evaluate_number(arguments[1], "PanelCut gap depth", evaluator,
			                     &gap_depth)) return result;
			gap_seen = true;
		}
		else if (option.optionName() == "up") {
			if (!evaluate_number(arguments[0], "PanelCut up X", evaluator,
			                     &candidate.up_hint.x) ||
			    !evaluate_number(arguments[1], "PanelCut up Y", evaluator,
			                     &candidate.up_hint.y) ||
			    !evaluate_number(arguments[2], "PanelCut up Z", evaluator,
			                     &candidate.up_hint.z)) return result;
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.cap_policy, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (!path_seen || !gap_seen) {
		result.diagnostic = "PanelCut requires path(...) and gap(...).";
		return result;
	}
	const std::shared_ptr<const Profile2D> gap_profile =
		Profile2DFactory().createRectangle(gap_width, gap_depth, &result.diagnostic);
	if (!gap_profile) return result;
	copy_profile_to_candidate(*gap_profile, &candidate.profile);
	result.specification = SweepProfileSpecificationValidator().validatePanelCut(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_nested_source_shape(
	const NestedSourceShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	if (!descriptor.sourceDescriptor()) {
		result.diagnostic = descriptor.shapeName() + " source descriptor is missing.";
		return result;
	}
	ShapeSpecificationEvaluationResult source_result =
		ShapeSpecificationEvaluator().evaluate(*descriptor.sourceDescriptor(), evaluator);
	if (!source_result.succeeded()) {
		result.diagnostic = descriptor.shapeName() + " source failed: " +
		                    source_result.diagnostic;
		return result;
	}

	GeometryDetailLevel detail_level = source_result.specification->detailLevel();
	if (descriptor.shapeName() == "MirrorShape") {
		int plane_axis = -1;
		float plane_offset = 0.0f;
		MirrorShapeMode mode = MirrorShapeMode::SourceAndMirrored;
		bool plane_seen = false;
		for (const ShapeOptionSyntax &option : descriptor.options()) {
			const auto &arguments = option.arguments();
			if (option.optionName() == "plane") {
				const std::string axis_name = lowercase_copy(arguments[0].sourceText());
				if (axis_name == "x") plane_axis = 0;
				else if (axis_name == "y") plane_axis = 1;
				else if (axis_name == "z") plane_axis = 2;
				else {
					result.diagnostic = "MirrorShape plane axis must be x, y, or z.";
					return result;
				}
				if (!evaluate_number(arguments[1], "MirrorShape plane offset", evaluator,
				                     &plane_offset)) return result;
				plane_seen = true;
			}
			else if (option.optionName() == "mode") {
				const std::string mode_name = lowercase_copy(arguments[0].sourceText());
				if (mode_name == "mirrored" || mode_name == "mirroredonly") {
					mode = MirrorShapeMode::MirroredOnly;
				}
				else if (mode_name == "both" || mode_name == "sourceandmirrored") {
					mode = MirrorShapeMode::SourceAndMirrored;
				}
				else {
					result.diagnostic = "MirrorShape mode must be mirrored or both.";
					return result;
				}
			}
			else if (option.optionName() == "detail" &&
			         !parse_geometry_detail_level(
					         arguments[0], &detail_level, &result.diagnostic)) return result;
		}
		if (!plane_seen) {
			result.diagnostic = "MirrorShape requires plane(axis offset).";
			return result;
		}
		std::ostringstream key;
		key << std::hexfloat << "MirrorShape:v1:source="
		    << source_result.specification->key().canonicalValue()
		    << ":axis=" << plane_axis << ":offset=" << plane_offset
		    << ":mode=" << static_cast<int>(mode)
		    << ":detail=" << geometryDetailLevelRank(detail_level);
		result.specification = std::make_shared<const MirrorShapeSpecification>(
			source_result.specification, plane_axis, plane_offset, mode,
			ShapeSpecificationKey(key.str()), descriptor.canonicalText(), detail_level);
		return result;
	}

	float thickness = 0.0f;
	ShellOffsetSide side = ShellOffsetSide::Both;
	bool thickness_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "thickness") {
			if (!evaluate_number(arguments[0], "ShellOffset thickness", evaluator,
			                     &thickness)) return result;
			thickness_seen = true;
		}
		else if (option.optionName() == "side") {
			const std::string side_name = lowercase_copy(arguments[0].sourceText());
			if (side_name == "outward") side = ShellOffsetSide::Outward;
			else if (side_name == "inward") side = ShellOffsetSide::Inward;
			else if (side_name == "both") side = ShellOffsetSide::Both;
			else {
				result.diagnostic = "ShellOffset side must be outward, inward, or both.";
				return result;
			}
		}
		else if (option.optionName() == "detail" &&
		         !parse_geometry_detail_level(
			         arguments[0], &detail_level, &result.diagnostic)) return result;
	}
	if (!thickness_seen || thickness <= 0.0f) {
		result.diagnostic = "ShellOffset requires positive thickness(...).";
		return result;
	}
	std::ostringstream key;
	key << std::hexfloat << "ShellOffset:v1:source="
	    << source_result.specification->key().canonicalValue()
	    << ":thickness=" << thickness << ":side=" << static_cast<int>(side)
	    << ":detail=" << geometryDetailLevelRank(detail_level);
	result.specification = std::make_shared<const ShellOffsetShapeSpecification>(
		source_result.specification, thickness, side,
		ShapeSpecificationKey(key.str()), descriptor.canonicalText(), detail_level);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_instance_array(
	const InstanceArrayDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	if (!descriptor.sourceDescriptor()) {
		result.diagnostic = "InstanceArray source descriptor is missing.";
		return result;
	}
	ShapeSpecificationEvaluationResult source_result =
		ShapeSpecificationEvaluator().evaluate(*descriptor.sourceDescriptor(), evaluator);
	if (!source_result.succeeded()) {
		result.diagnostic = "InstanceArray source failed: " + source_result.diagnostic;
		return result;
	}

	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
	if (!parse_geometry_detail_level(
			descriptor.detail(), &detail_level, &result.diagnostic)) return result;

	const auto &arguments = descriptor.pattern().arguments();
	InstanceArraySpecification array({});
	if (descriptor.pattern().kind() == InstanceArrayPatternKind::Linear) {
		int count = 0;
		glm::vec3 spacing(0.0f);
		if (!evaluate_integer(arguments[0], "InstanceArray linear count", evaluator,
		                      &count, &result.diagnostic) ||
		    !evaluate_number(arguments[1], "InstanceArray linear spacing X", evaluator,
		                     &spacing.x) ||
		    !evaluate_number(arguments[2], "InstanceArray linear spacing Y", evaluator,
		                     &spacing.y) ||
		    !evaluate_number(arguments[3], "InstanceArray linear spacing Z", evaluator,
		                     &spacing.z)) return result;
		if (count <= 0) {
			result.diagnostic = "InstanceArray linear count must be positive.";
			return result;
		}
		array = InstanceArraySpecification::createLinear(
			static_cast<std::size_t>(count), spacing);
	}
	else if (descriptor.pattern().kind() == InstanceArrayPatternKind::Grid) {
		int x_count = 0;
		int y_count = 0;
		glm::vec2 spacing(0.0f);
		if (!evaluate_integer(arguments[0], "InstanceArray grid X count", evaluator,
		                      &x_count, &result.diagnostic) ||
		    !evaluate_integer(arguments[1], "InstanceArray grid Y count", evaluator,
		                      &y_count, &result.diagnostic) ||
		    !evaluate_number(arguments[2], "InstanceArray grid spacing X", evaluator,
		                     &spacing.x) ||
		    !evaluate_number(arguments[3], "InstanceArray grid spacing Y", evaluator,
		                     &spacing.y)) return result;
		if (x_count <= 0 || y_count <= 0) {
			result.diagnostic = "InstanceArray grid counts must be positive.";
			return result;
		}
		const std::size_t x_count_value = static_cast<std::size_t>(x_count);
		const std::size_t y_count_value = static_cast<std::size_t>(y_count);
		if (x_count_value > GeometryComplexityLimits().maximumInstanceArrayCount() /
		                    y_count_value) {
			result.diagnostic = "InstanceArray grid count exceeds the configured instance limit.";
			return result;
		}
		array = InstanceArraySpecification::createGrid(
			x_count_value, y_count_value, spacing);
	}
	else {
		int count = 0;
		float radius = 0.0f;
		glm::vec3 axis(0.0f);
		float start_degrees = 0.0f;
		if (!evaluate_integer(arguments[0], "InstanceArray radial count", evaluator,
		                      &count, &result.diagnostic) ||
		    !evaluate_number(arguments[1], "InstanceArray radial radius", evaluator,
		                     &radius) ||
		    !evaluate_number(arguments[2], "InstanceArray radial axis X", evaluator,
		                     &axis.x) ||
		    !evaluate_number(arguments[3], "InstanceArray radial axis Y", evaluator,
		                     &axis.y) ||
		    !evaluate_number(arguments[4], "InstanceArray radial axis Z", evaluator,
		                     &axis.z) ||
		    !evaluate_number(arguments[5], "InstanceArray radial start angle", evaluator,
		                     &start_degrees)) return result;
		if (count <= 0) {
			result.diagnostic = "InstanceArray radial count must be positive.";
			return result;
		}
		array = InstanceArraySpecification::createRadial(
			static_cast<std::size_t>(count), radius, axis, start_degrees);
	}

	const GeometryComplexityLimits limits;
	if (array.transforms().empty() ||
	    array.transforms().size() > limits.maximumInstanceArrayCount()) {
		result.diagnostic = "InstanceArray pattern is invalid or exceeds the configured instance limit.";
		return result;
	}

	std::ostringstream key;
	key << "InstanceArray:v1:source="
	    << source_result.specification->key().canonicalValue()
	    << ":detail=" << geometryDetailLevelRank(detail_level)
	    << ":transforms=" << std::hexfloat;
	for (const glm::mat4 &transform : array.transforms()) {
		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) {
				key << transform[column][row] << ",";
			}
		}
		key << ";";
	}
	result.specification = std::make_shared<const InstanceArrayShapeSpecification>(
		source_result.specification,
		std::move(array),
		ShapeSpecificationKey(key.str()),
		descriptor.canonicalText(),
		detail_level);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_compound_shape(
	const CompoundShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	CompoundShapeSpecificationCandidate candidate;
	if (!parse_geometry_detail_level(
			descriptor.detail(), &candidate.detail_level, &result.diagnostic)) {
		return result;
	}
	for (const CompoundShapePartSyntax &part : descriptor.parts()) {
		const ShapeSpecificationEvaluationResult source_result =
			ShapeSpecificationEvaluator().evaluate(*part.sourceDescriptor(), evaluator);
		if (!source_result.succeeded()) {
			result.diagnostic = "CompoundShape part '" + part.purpose() +
			                    "' source failed: " + source_result.diagnostic;
			return result;
		}
		CompoundShapePartCandidate part_candidate;
		part_candidate.purpose = part.purpose();
		part_candidate.shape = source_result.specification;
		for (std::size_t value_index = 0;
		     value_index < part.transformArguments().size();
		     ++value_index) {
			float value = 0.0f;
			if (!evaluate_number(
					part.transformArguments()[value_index],
					"CompoundShape transform value", evaluator, &value)) return result;
			const int column = static_cast<int>(value_index / 4u);
			const int row = static_cast<int>(value_index % 4u);
			part_candidate.local_transform[column][row] = value;
		}
		candidate.parts.push_back(std::move(part_candidate));
	}
	result.specification = CompoundShapeSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

std::vector<glm::vec2> create_symmetric_vehicle_section(
	float half_width,
	float bottom_y,
	float shoulder_y,
	float belt_y,
	float roof_y,
	float greenhouse_half_width)
{
	const std::vector<glm::vec2> right_half = {
		{0.0f, bottom_y},
		{half_width * 0.58f, bottom_y + 0.018f},
		{half_width, bottom_y + 0.075f},
		{half_width, shoulder_y},
		{half_width * 0.88f, belt_y},
		{greenhouse_half_width, roof_y * 0.88f},
		{0.0f, roof_y},
	};
	std::vector<glm::vec2> profile = right_half;
	for (std::size_t index = right_half.size() - 1u; index-- > 1u;) {
		profile.emplace_back(-right_half[index].x, right_half[index].y);
	}
	return profile;
}

std::vector<glm::vec2> create_vehicle_inner_section(
	const std::vector<glm::vec2> &outer,
	float shell_thickness)
{
	float maximum_x = 0.0f;
	float minimum_y = outer.front().y;
	float maximum_y = outer.front().y;
	for (const glm::vec2 &point : outer) {
		maximum_x = std::max(maximum_x, std::fabs(point.x));
		minimum_y = std::min(minimum_y, point.y);
		maximum_y = std::max(maximum_y, point.y);
	}
	const float profile_height = maximum_y - minimum_y;
	if (maximum_x <= shell_thickness * 2.0f ||
	    profile_height <= shell_thickness * 4.0f) return {};
	const float center_y = (minimum_y + maximum_y) * 0.5f;
	const float x_scale = (maximum_x - shell_thickness) / maximum_x;
	const float y_scale = (profile_height - shell_thickness * 2.0f) / profile_height;
	std::vector<glm::vec2> inner;
	inner.reserve(outer.size());
	for (const glm::vec2 &point : outer) {
		inner.emplace_back(
			point.x * x_scale,
			center_y + (point.y - center_y) * y_scale);
	}
	return inner;
}

ShapeSpecificationEvaluationResult evaluate_vehicle_body_shell(
	const ShapeDescriptorSyntax &descriptor,
	const ShapeSpecificationEvaluator::ExpressionEvaluator &evaluator)
{
	ShapeSpecificationEvaluationResult result;
	ShellLoftShapeSpecificationCandidate candidate;
	float shell_thickness = 0.0f;
	bool thickness_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "thickness") {
			if (!evaluate_number(arguments[0], "VehicleBodyShell thickness", evaluator,
			                     &shell_thickness)) return result;
			thickness_seen = true;
		}
		else if (option.optionName() == "section") {
			float station_z = 0.0f;
			float half_width = 0.0f;
			float bottom_y = 0.0f;
			float shoulder_y = 0.0f;
			float belt_y = 0.0f;
			float roof_y = 0.0f;
			float greenhouse_half_width = 0.0f;
			if (!evaluate_number(arguments[0], "VehicleBodyShell station Z", evaluator, &station_z) ||
			    !evaluate_number(arguments[1], "VehicleBodyShell half width", evaluator, &half_width) ||
			    !evaluate_number(arguments[2], "VehicleBodyShell bottom Y", evaluator, &bottom_y) ||
			    !evaluate_number(arguments[3], "VehicleBodyShell shoulder Y", evaluator, &shoulder_y) ||
			    !evaluate_number(arguments[4], "VehicleBodyShell belt Y", evaluator, &belt_y) ||
			    !evaluate_number(arguments[5], "VehicleBodyShell roof Y", evaluator, &roof_y) ||
			    !evaluate_number(arguments[6], "VehicleBodyShell greenhouse half width", evaluator,
			                     &greenhouse_half_width)) return result;
			if (half_width <= 0.0f || greenhouse_half_width < 0.0f ||
			    greenhouse_half_width >= half_width || bottom_y >= shoulder_y ||
			    shoulder_y > belt_y || belt_y >= roof_y) {
				result.diagnostic = "VehicleBodyShell sections require positive width, greenhouse width inside the body, and bottom < shoulder <= belt < roof.";
				return result;
			}
			ShellLoftSectionCandidate section;
			section.axial_position = station_z;
			section.outer_loop = create_symmetric_vehicle_section(
				half_width, bottom_y, shoulder_y, belt_y, roof_y,
				greenhouse_half_width);
			candidate.sections.push_back(std::move(section));
		}
		else if (option.optionName() == "cap") {
			if (!parse_extrude_cap_policy(
					arguments[0], &candidate.cap_policy, &result.diagnostic)) return result;
		}
		else if (option.optionName() == "detail") {
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) return result;
		}
	}
	if (!thickness_seen || !std::isfinite(shell_thickness) || shell_thickness <= 0.0f ||
	    candidate.sections.size() < 2u) {
		result.diagnostic = "VehicleBodyShell requires positive thickness and at least two section(...) records.";
		return result;
	}
	for (ShellLoftSectionCandidate &section : candidate.sections) {
		section.inner_loop = create_vehicle_inner_section(
			section.outer_loop, shell_thickness);
		if (section.inner_loop.empty()) {
			result.diagnostic = "VehicleBodyShell thickness is too large for at least one section.";
			return result;
		}
	}
	result.specification = LoftSpecificationValidator().validateShellLoft(
		std::move(candidate), &result.diagnostic);
	return result;
}

ShapeSpecificationEvaluationResult evaluate_generated_mesh_reference(
	const ShapeDescriptorSyntax &descriptor)
{
	ShapeSpecificationEvaluationResult result;
	GeneratedMeshReferenceSpecificationCandidate candidate;
	bool mesh_key_seen = false;
	bool detail_seen = false;
	bool topology_seen = false;
	for (const ShapeOptionSyntax &option : descriptor.options()) {
		const auto &arguments = option.arguments();
		if (option.optionName() == "meshKey") {
			if (mesh_key_seen) {
				result.diagnostic =
					"GeneratedMeshReference meshKey may be specified once.";
				return result;
			}
			candidate.mesh_key = arguments[0].sourceText();
			mesh_key_seen = true;
		}
		else if (option.optionName() == "detail") {
			if (detail_seen) {
				result.diagnostic =
					"GeneratedMeshReference detail may be specified once.";
				return result;
			}
			if (!parse_geometry_detail_level(
					arguments[0], &candidate.detail_level, &result.diagnostic)) {
				return result;
			}
			detail_seen = true;
		}
		else if (option.optionName() == "topology") {
			if (topology_seen) {
				result.diagnostic =
					"GeneratedMeshReference topology may be specified once.";
				return result;
			}
			if (!parse_topology(
					arguments[0], &candidate.topology, &result.diagnostic)) {
				return result;
			}
			topology_seen = true;
		}
	}
	if (!mesh_key_seen) {
		result.diagnostic = "GeneratedMeshReference requires meshKey(...).";
		return result;
	}
	result.specification = GeneratedMeshReferenceSpecificationValidator().validate(
		std::move(candidate), &result.diagnostic);
	return result;
}

}

ShapeSpecificationEvaluationResult ShapeSpecificationEvaluator::evaluate(
	const ShapeDescriptorSyntax &descriptor,
	const ExpressionEvaluator &expression_evaluator) const
{
	if (descriptor.shapeName() == "AxialProfile") {
		const auto *axial_profile =
			dynamic_cast<const AxialProfileDescriptorSyntax *>(&descriptor);
		if (axial_profile == nullptr) {
			ShapeSpecificationEvaluationResult result;
			result.diagnostic = "AxialProfile descriptor has an invalid syntax model.";
			return result;
		}
		return evaluate_axial_profile(*axial_profile, expression_evaluator);
	}
	if (descriptor.shapeName() == "Extrude") {
		const auto *extrude =
			dynamic_cast<const ExtrudeProfileDescriptorSyntax *>(&descriptor);
		if (extrude == nullptr) {
			ShapeSpecificationEvaluationResult result;
			result.diagnostic = "Extrude descriptor has an invalid syntax model.";
			return result;
		}
		return evaluate_extrude_profile(*extrude, expression_evaluator);
	}
	if (descriptor.shapeName() == "SweepProfile") {
		return evaluate_sweep_profile(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "VariableSectionSweep") {
		return evaluate_variable_section_sweep(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "SweepDisk") {
		return evaluate_sweep_disk(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "Revolve") {
		return evaluate_revolve(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "Loft") {
		return evaluate_loft(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "SurfaceLoft") {
		return evaluate_surface_loft(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "CurveNetworkSurface") {
		return evaluate_curve_network_surface(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "ShellLoft") {
		return evaluate_shell_loft(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "FoldedProfile") {
		return evaluate_folded_profile(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "CurvedPanel") {
		return evaluate_curved_panel(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "FormedPanel") {
		return evaluate_formed_panel(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "HostedOpening") {
		return evaluate_hosted_opening(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "EmbossedBead") {
		return evaluate_embossed_bead(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "EdgeFlange") {
		return evaluate_edge_flange(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "PanelCut") {
		return evaluate_panel_cut(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "MirrorShape" ||
	    descriptor.shapeName() == "ShellOffset") {
		const auto *nested_source =
			dynamic_cast<const NestedSourceShapeDescriptorSyntax *>(&descriptor);
		if (nested_source == nullptr) {
			ShapeSpecificationEvaluationResult result;
			result.diagnostic = descriptor.shapeName() +
			                    " descriptor has an invalid syntax model.";
			return result;
		}
		return evaluate_nested_source_shape(*nested_source, expression_evaluator);
	}
	if (descriptor.shapeName() == "InstanceArray") {
		const auto *instance_array =
			dynamic_cast<const InstanceArrayDescriptorSyntax *>(&descriptor);
		if (instance_array == nullptr) {
			ShapeSpecificationEvaluationResult result;
			result.diagnostic = "InstanceArray descriptor has an invalid syntax model.";
			return result;
		}
		return evaluate_instance_array(*instance_array, expression_evaluator);
	}
	if (descriptor.shapeName() == "CompoundShape") {
		const auto *compound =
			dynamic_cast<const CompoundShapeDescriptorSyntax *>(&descriptor);
		if (compound == nullptr) {
			ShapeSpecificationEvaluationResult result;
			result.diagnostic = "CompoundShape descriptor has an invalid syntax model.";
			return result;
		}
		return evaluate_compound_shape(*compound, expression_evaluator);
	}
	if (descriptor.shapeName() == "VehicleBodyShell") {
		return evaluate_vehicle_body_shell(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "GeneratedMeshReference") {
		return evaluate_generated_mesh_reference(descriptor);
	}
	if (descriptor.shapeName() == "TaperedSweep") {
		return evaluate_tapered_sweep(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "BranchJunction") {
		return evaluate_branch_junction(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "LeafBlade" ||
	    descriptor.shapeName() == "PetalBlade") {
		return evaluate_botanical_blade(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "Plant") {
		return evaluate_plant(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "Vine") {
		return evaluate_vine(descriptor, expression_evaluator);
	}
	if (descriptor.shapeName() == "ScatterRegion") {
		return evaluate_scatter_region(descriptor, expression_evaluator);
	}
	ShapeAliasExpansionService alias_expansion;
	ShapeSpecificationEvaluationResult result;
	auto canonical_descriptor = alias_expansion.expandAlias(
		descriptor, &result.diagnostic);
	if (!canonical_descriptor) return result;
	if (canonical_descriptor->shapeName() == "Cylinder") {
		return evaluate_cylinder(*canonical_descriptor, expression_evaluator);
	}
	if (canonical_descriptor->shapeName() == "Sphere") {
		return evaluate_sphere(*canonical_descriptor, expression_evaluator);
	}
	result.diagnostic = "Unsupported canonical shape family '" +
	                    canonical_descriptor->shapeName() + "'.";
	return result;
}
