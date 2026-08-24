#include "vegetation/service/PlantShapeSpecificationValidator.h"

#include "geometry/model/BotanicalBladeProfile.h"
#include "vegetation/model/PhyllotaxisMode.h"
#include "vegetation/service/CrownVolumeContainmentService.h"
#include "vegetation/service/PlantLSystemSpecificationValidationService.h"
#include "vegetation/service/SpaceColonizationSpecificationValidationService.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

bool finite_positive(float value)
{
	return std::isfinite(value) && value > 0.0f;
}

bool valid_growth_curve(GrowthCurve curve)
{
	return curve >= GrowthCurve::Linear && curve <= GrowthCurve::EaseOut;
}

bool valid_growth_channel(const PlantGrowthChannelSpecification &channel)
{
	return finite_positive(channel.initialFactor()) &&
	       channel.initialFactor() <= 1.0f &&
	       valid_growth_curve(channel.curve());
}

bool valid_growth(const PlantGrowthSpecification &growth)
{
	return std::isfinite(growth.startTime()) &&
	       finite_positive(growth.duration()) &&
	       valid_growth_channel(growth.lengthGrowth()) &&
	       valid_growth_channel(growth.radiusGrowth()) &&
	       valid_growth_channel(growth.organGrowth());
}

bool valid_compound_leaf(
	const PlantCompoundLeafSpecification &compound_leaf)
{
	if (!compound_leaf.isEnabled()) return true;
	const bool supported_pattern =
		compound_leaf.pattern() == PhyllotaxisMode::Alternate ||
		compound_leaf.pattern() == PhyllotaxisMode::Opposite;
	return compound_leaf.leafletNodeCount() >= 2 &&
	       finite_positive(compound_leaf.rachisLength()) &&
	       finite_positive(compound_leaf.rachisBaseRadius()) &&
	       finite_positive(compound_leaf.rachisTipRadius()) &&
	       compound_leaf.rachisTipRadius() <=
		       compound_leaf.rachisBaseRadius() &&
	       supported_pattern &&
	       std::isfinite(compound_leaf.divergenceDegrees()) &&
	       std::isfinite(compound_leaf.orientationUpBias()) &&
	       compound_leaf.orientationUpBias() >= 0.0f &&
	       finite_positive(compound_leaf.leafletScale());
}

bool supported_organ_array_type(VegetationOrganType organ_type)
{
	return organ_type == VegetationOrganType::Bud ||
	       organ_type == VegetationOrganType::Leaf ||
	       organ_type == VegetationOrganType::Petal ||
	       organ_type == VegetationOrganType::Flower ||
	       organ_type == VegetationOrganType::Fruit ||
	       organ_type == VegetationOrganType::Thorn;
}

bool valid_organ_array(const PlantOrganArraySpecification &organ_array)
{
	const bool path_host = organ_array.host() == VegetationOrganArrayHost::Stem ||
	                       organ_array.host() == VegetationOrganArrayHost::Branch;
	return !organ_array.identifier().empty() &&
	       supported_organ_array_type(organ_array.organType()) &&
	       organ_array.count() >= 1 &&
	       std::isfinite(organ_array.spacing()) &&
	       organ_array.spacing() >= 0.0f &&
	       (!path_host || organ_array.count() == 1 ||
	        organ_array.spacing() > 0.0f) &&
	       std::isfinite(organ_array.azimuthProgressionDegrees()) &&
	       finite_positive(organ_array.initialScale()) &&
	       finite_positive(organ_array.scaleFalloff()) &&
	       std::isfinite(organ_array.jitterFraction()) &&
	       organ_array.jitterFraction() >= 0.0f &&
	       organ_array.jitterFraction() <= 1.0f &&
	       organ_array.minimumBranchOrder() >= 0 &&
	       (organ_array.orientation() !=
		        VegetationOrganOrientation::SurfaceNormal ||
	        organ_array.host() == VegetationOrganArrayHost::Surface);
}

bool valid_fruit(const PlantFruitSpecification &fruit)
{
	return !fruit.isEnabled() ||
	       (finite_positive(fruit.height()) &&
	        finite_positive(fruit.maximumRadius()) &&
	        std::isfinite(fruit.shoulderFraction()) &&
	        fruit.shoulderFraction() >= 0.20f &&
	        fruit.shoulderFraction() <= 0.80f &&
	        std::isfinite(fruit.fullness()) && fruit.fullness() >= 2.0f &&
	        fruit.fullness() <= 12.0f && fruit.radialSegments() >= 6 &&
	        fruit.profileSegments() >= 4);
}

bool valid_flower_head(const FlowerHeadSpecification &flower_head)
{
	return !flower_head.isEnabled() ||
	       (flower_head.floretCount() >= 1 &&
	        std::isfinite(flower_head.radius()) && flower_head.radius() >= 0.0f &&
	        std::isfinite(flower_head.divergenceDegrees()) &&
	        std::isfinite(flower_head.phaseDegrees()) &&
	        std::isfinite(flower_head.tiltDegrees()) &&
	        finite_positive(flower_head.scaleFalloff()));
}

bool valid_inflorescence(const InflorescenceSpecification &inflorescence)
{
	return !inflorescence.isEnabled() ||
	       (inflorescence.flowerCount() >= 1 &&
	        std::isfinite(inflorescence.spacing()) &&
	        inflorescence.spacing() >= 0.0f &&
	        std::isfinite(inflorescence.radialExtent()) &&
	        inflorescence.radialExtent() >= 0.0f &&
	        std::isfinite(inflorescence.phaseDegrees()) &&
	        std::isfinite(inflorescence.tiltDegrees()) &&
	        finite_positive(inflorescence.scaleFalloff()));
}

bool valid_tropism(const TropismInfluence &influence)
{
	return std::isfinite(influence.direction().x) &&
	       std::isfinite(influence.direction().y) &&
	       std::isfinite(influence.direction().z) &&
	       std::isfinite(influence.weight()) && influence.weight() >= 0.0f &&
	       (influence.weight() == 0.0f ||
	        glm::length(influence.direction()) > 1.0e-6f);
}

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool valid_bounds(const AxisAlignedBounds &bounds)
{
	return bounds.valid && finite(bounds.min) && finite(bounds.max) &&
	       bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y &&
	       bounds.min.z <= bounds.max.z;
}

std::string float_bits(float value);

void append_crown_canonical(
	std::ostringstream *text,
	const CrownVolumeSpecification &crown)
{
	if (crown.kind() == CrownVolumeKind::CustomSampled) {
		*text << " crownCustom(" << crown.customSampleRadius() << ")";
		for (const glm::vec3 &sample : crown.customSamples()) {
			*text << " crownSample(" << sample.x << " " << sample.y << " "
			      << sample.z << ")";
		}
		return;
	}
	*text << " crown(" << crownVolumeKindName(crown.kind()) << " "
	      << crown.origin().x << " " << crown.origin().y << " "
	      << crown.origin().z << " " << crown.dimensions().x << " "
	      << crown.dimensions().y << " " << crown.dimensions().z;
	if (crown.kind() == CrownVolumeKind::Lobed) {
		*text << " " << crown.lobeCount() << " " << crown.lobeAmplitude();
	}
	*text << ")";
}

void append_crown_key(
	std::ostringstream *key,
	const CrownVolumeSpecification &crown)
{
	*key << ":crownKind=" << static_cast<int>(crown.kind())
	     << ":crownOrigin=" << float_bits(crown.origin().x) << ","
	     << float_bits(crown.origin().y) << ","
	     << float_bits(crown.origin().z)
	     << ":crownDimensions=" << float_bits(crown.dimensions().x) << ","
	     << float_bits(crown.dimensions().y) << ","
	     << float_bits(crown.dimensions().z)
	     << ":crownLobes=" << crown.lobeCount()
	     << ":crownLobeAmplitude=" << float_bits(crown.lobeAmplitude())
	     << ":crownSampleRadius=" << float_bits(crown.customSampleRadius());
	for (const glm::vec3 &sample : crown.customSamples()) {
		*key << ":crownSample=" << float_bits(sample.x) << ","
		     << float_bits(sample.y) << "," << float_bits(sample.z);
	}
}

std::string float_bits(float value)
{
	if (value == 0.0f) value = 0.0f;
	std::uint32_t bits = 0u;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

const char *architecture_name(PlantArchitecture architecture)
{
	switch (architecture) {
	case PlantArchitecture::Tree: return "Tree";
	case PlantArchitecture::Shrub: return "Shrub";
	case PlantArchitecture::Herb: return "Herb";
	case PlantArchitecture::Grass: return "Grass";
	case PlantArchitecture::Vine: return "Vine";
	}
	return "Unknown";
}

const char *development_state_name(PlantDevelopmentState state)
{
	switch (state) {
	case PlantDevelopmentState::Seed: return "Seed";
	case PlantDevelopmentState::Bud: return "Bud";
	case PlantDevelopmentState::Shoot: return "Shoot";
	case PlantDevelopmentState::Juvenile: return "Juvenile";
	case PlantDevelopmentState::Mature: return "Mature";
	case PlantDevelopmentState::Flowering: return "Flowering";
	case PlantDevelopmentState::Fruiting: return "Fruiting";
	case PlantDevelopmentState::Senescent: return "Senescent";
	case PlantDevelopmentState::Dormant: return "Dormant";
	case PlantDevelopmentState::Dead: return "Dead";
	}
	return "Unknown";
}

const char *phyllotaxis_name(PhyllotaxisMode mode)
{
	switch (mode) {
	case PhyllotaxisMode::Alternate: return "Alternate";
	case PhyllotaxisMode::Opposite: return "Opposite";
	case PhyllotaxisMode::Decussate: return "Decussate";
	case PhyllotaxisMode::Whorled: return "Whorled";
	case PhyllotaxisMode::Spiral: return "Spiral";
	case PhyllotaxisMode::Rosette: return "Rosette";
	}
	return "Unknown";
}

bool valid_species(const PlantSpeciesSpecification &species)
{
	const PlantBranchingSpecification &branching = species.branching();
	const PlantLeafSpecification &leaf = species.leaf();
	const PlantPetioleSpecification &petiole = leaf.petiole();
	const PlantCompoundLeafSpecification &compound_leaf = leaf.compoundLeaf();
	const PlantFlowerSpecification &flower = species.flower();
	const PlantFruitSpecification &fruit = species.fruit();
	bool requires_fruit = false;
	for (const PlantOrganArraySpecification &organ_array :
	     species.organArrays()) {
		if (!valid_organ_array(organ_array)) return false;
		if (organ_array.organType() == VegetationOrganType::Fruit) {
			requires_fruit = true;
		}
	}
	const bool flower_is_valid = !flower.isEnabled() ||
		(finite_positive(flower.petalLength()) &&
		 finite_positive(flower.petalWidth()) &&
		 std::isfinite(flower.longitudinalCurvature()) &&
		 std::isfinite(flower.camber()) &&
		 std::isfinite(flower.twistDegrees()) &&
		 std::isfinite(flower.thickness()) && flower.thickness() >= 0.0f &&
		 finite_positive(flower.widthPower()) &&
		 flower.petalWhorl().organCount() >= 1 &&
		 std::isfinite(flower.petalWhorl().radius()) &&
		 flower.petalWhorl().radius() >= 0.0f &&
		 std::isfinite(flower.petalWhorl().phaseDegrees()) &&
		 std::isfinite(flower.petalWhorl().tiltDegrees()) &&
		 valid_flower_head(flower.flowerHead()) &&
		 valid_inflorescence(flower.inflorescence()) &&
		 !(flower.flowerHead().isEnabled() &&
		   flower.inflorescence().isEnabled()));
	return !species.identifier().empty() &&
	       finite_positive(branching.primaryAxisLength()) &&
	       finite_positive(branching.baseRadius()) &&
	       finite_positive(branching.tipRadius()) &&
	       branching.tipRadius() <= branching.baseRadius() &&
	       branching.primaryInternodeCount() >= 1 &&
	       branching.maximumBranchOrder() >= 0 &&
	       branching.lateralBranchesPerNode() >= 0 &&
	       branching.branchEveryNInternodes() >= 1 &&
	       std::isfinite(branching.branchAngleDegrees()) &&
	       std::isfinite(branching.azimuthDivergenceDegrees()) &&
	       finite_positive(branching.branchLengthFraction()) &&
	       finite_positive(branching.branchLengthFalloff()) &&
	       finite_positive(branching.continuationRadiusRatio()) &&
	       branching.continuationRadiusRatio() < 1.0f &&
	       finite_positive(branching.radiusConservationGamma()) &&
	       std::isfinite(branching.directionVariationDegrees()) &&
	       branching.directionVariationDegrees() >= 0.0f &&
	       std::isfinite(branching.lengthVariationFraction()) &&
	       branching.lengthVariationFraction() >= 0.0f &&
	       std::isfinite(branching.upwardTropismWeight()) &&
	       branching.upwardTropismWeight() >= 0.0f &&
	       finite_positive(leaf.length()) && finite_positive(leaf.width()) &&
	       std::isfinite(leaf.camber()) &&
	       std::isfinite(leaf.twistDegrees()) &&
	       std::isfinite(leaf.thickness()) && leaf.thickness() >= 0.0f &&
	       leaf.minimumBranchOrder() >= 0 &&
	       finite_positive(petiole.length()) &&
	       finite_positive(petiole.baseRadius()) &&
	       finite_positive(petiole.tipRadius()) &&
	       petiole.tipRadius() <= petiole.baseRadius() &&
	       valid_compound_leaf(compound_leaf) &&
	       std::isfinite(leaf.phyllotaxis().divergenceDegrees()) &&
	       finite_positive(leaf.phyllotaxis().internodeLength()) &&
	       leaf.phyllotaxis().organsPerNode() >= 1 &&
	       std::isfinite(species.floweringAge()) &&
	       std::isfinite(species.matureAge()) &&
	       species.floweringAge() >= 0.0f && species.matureAge() > 0.0f &&
	       flower_is_valid && valid_fruit(fruit) &&
	       (!requires_fruit || fruit.isEnabled());
}

} // namespace

std::shared_ptr<const PlantShapeSpecification>
PlantShapeSpecificationValidator::validate(
	PlantShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!candidate.species.has_value()) {
		if (diagnostic != nullptr) *diagnostic = "Plant requires species(name).";
		return {};
	}
	if (!valid_compound_leaf(candidate.species->leaf().compoundLeaf())) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Plant leafArray requires at least two nodes, a tapered positive rachis, Alternate or Opposite placement, non-negative up bias, and positive leaflet scale.";
		}
		return {};
	}
	for (const PlantOrganArraySpecification &organ_array :
	     candidate.species->organArrays()) {
		if (!valid_organ_array(organ_array)) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"Plant organArray requires a supported organ and host, positive count and scale, finite spacing and azimuth, jitter in [0,1], and SurfaceNormal only on Surface hosts.";
			}
			return {};
		}
		if (static_cast<std::size_t>(organ_array.count()) >
		    vegetation_complexity_limits_.maximumOrganAttachments()) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"Plant organArray count exceeds the organ-attachment safety limit.";
			}
			return {};
		}
	}
	const PlantFlowerSpecification &validated_flower = candidate.species->flower();
	if (validated_flower.flowerHead().isEnabled() &&
	    validated_flower.inflorescence().isEnabled()) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Plant flower topology must select either flowerHead or inflorescence, not both.";
		}
		return {};
	}
	if (!valid_flower_head(validated_flower.flowerHead())) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Plant flowerHead requires a positive floret count, non-negative radius, finite angles, and positive scale falloff.";
		}
		return {};
	}
	if (validated_flower.flowerHead().isEnabled() &&
	    static_cast<std::size_t>(validated_flower.flowerHead().floretCount()) >
		    vegetation_complexity_limits_.maximumOrganAttachments()) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Plant flowerHead count exceeds the organ-attachment safety limit.";
		}
		return {};
	}
	if (!valid_inflorescence(validated_flower.inflorescence())) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Plant inflorescence requires a positive flower count, non-negative spacing and radial extent, finite angles, and positive scale falloff.";
		}
		return {};
	}
	if (validated_flower.inflorescence().isEnabled() &&
	    static_cast<std::size_t>(validated_flower.inflorescence().flowerCount()) >
		    vegetation_complexity_limits_.maximumOrganAttachments()) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Plant inflorescence count exceeds the organ-attachment safety limit.";
		}
		return {};
	}
	if (!valid_species(*candidate.species)) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant species parameters are invalid or non-finite.";
		}
		return {};
	}
	if (!std::isfinite(candidate.age) || candidate.age < 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant age must be finite and non-negative.";
		}
		return {};
	}
	if (candidate.growth.has_value() && !valid_growth(*candidate.growth)) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant growth requires finite times, positive duration, and channel factors in (0,1].";
		}
		return {};
	}
	for (const TropismInfluence &influence : candidate.tropism_influences) {
		if (!valid_tropism(influence)) {
			if (diagnostic != nullptr) {
				*diagnostic = "Plant tropism requires a finite direction and non-negative weight.";
			}
			return {};
		}
	}
	if (candidate.species->branching().primaryInternodeCount() + 1 >
	    static_cast<int>(geometry_complexity_limits_.maximumPathPoints())) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant primary internode count exceeds the path-point limit.";
		}
		return {};
	}
	const PlantCompoundLeafSpecification &compound_leaf_limit =
		candidate.species->leaf().compoundLeaf();
	if (compound_leaf_limit.isEnabled() &&
	    compound_leaf_limit.leafletNodeCount() >
		    static_cast<int>(geometry_complexity_limits_.maximumPathPoints())) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant leafArray node count exceeds the path-point limit.";
		}
		return {};
	}
	const PlantTopologyGenerationSpecification &topology_generation =
		candidate.topology_generation;
	if (topology_generation.method() ==
	    PlantTopologyGenerationMethod::SpaceColonization) {
		if (!topology_generation.spaceColonization().has_value()) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"Plant SpaceColonization generation requires a crown and algorithm specification.";
			}
			return {};
		}
		const PlantSpaceColonizationSpecification &space_colonization =
			*topology_generation.spaceColonization();
		std::string topology_diagnostic;
		if (!CrownVolumeContainmentService().validate(
				space_colonization.crownVolume(), &topology_diagnostic) ||
		    !SpaceColonizationSpecificationValidationService(
				    vegetation_complexity_limits_)
				 .validate(space_colonization.colonization(), &topology_diagnostic)) {
			if (diagnostic != nullptr) *diagnostic = topology_diagnostic;
			return {};
		}
		for (const VegetationObstacleBoundary &obstacle :
		     space_colonization.obstacles()) {
			if (obstacle.identifier().empty() || !valid_bounds(obstacle.bounds()) ||
			    !std::isfinite(obstacle.clearance()) || obstacle.clearance() < 0.0f) {
				if (diagnostic != nullptr) {
					*diagnostic =
						"Plant crown obstacles require identifiers, finite bounds, and non-negative clearance.";
				}
				return {};
			}
		}
	}
	else if (topology_generation.method() ==
	         PlantTopologyGenerationMethod::LSystem) {
		if (!topology_generation.lSystem().has_value()) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"Plant LSystem generation requires an axiom, production rules, and turtle parameters.";
			}
			return {};
		}
		std::string topology_diagnostic;
		if (!PlantLSystemSpecificationValidationService(
				vegetation_complexity_limits_)
			 .validate(*topology_generation.lSystem(), &topology_diagnostic)) {
			if (diagnostic != nullptr) *diagnostic = topology_diagnostic;
			return {};
		}
	}

	const PlantSpeciesSpecification &species = *candidate.species;
	const PlantBranchingSpecification &branching = species.branching();
	const PlantLeafSpecification &leaf = species.leaf();
	const PlantPetioleSpecification &petiole = leaf.petiole();
	const PlantCompoundLeafSpecification &compound_leaf = leaf.compoundLeaf();
	const PhyllotaxisSpecification &phyllotaxis = leaf.phyllotaxis();
	const PlantFlowerSpecification &flower = species.flower();
	const PlantFruitSpecification &fruit = species.fruit();
	std::ostringstream canonical;
	canonical << "Plant(species(" << species.identifier() << ") architecture("
	          << architecture_name(species.architecture()) << ") age(" << candidate.age
	          << ") state(" << development_state_name(candidate.development_state)
	          << ") seed(" << candidate.deterministic_seed << ") generation("
	          << plantTopologyGenerationMethodName(topology_generation.method())
	          << ") trunk("
	          << branching.primaryAxisLength() << " " << branching.baseRadius()
	          << " " << branching.tipRadius() << " "
	          << branching.primaryInternodeCount() << ") branching("
	          << branching.maximumBranchOrder() << " "
	          << branching.lateralBranchesPerNode() << " "
	          << branching.branchEveryNInternodes() << " "
	          << branching.branchAngleDegrees() << " "
	          << branching.azimuthDivergenceDegrees() << " "
	          << branching.branchLengthFraction() << " "
	          << branching.branchLengthFalloff() << " "
	          << branching.continuationRadiusRatio() << " "
	          << branching.radiusConservationGamma() << " "
	          << branching.directionVariationDegrees() << " "
	          << branching.lengthVariationFraction() << " "
	          << branching.upwardTropismWeight() << ") leaf("
	          << botanicalBladeProfileName(leaf.profile()) << " " << leaf.length()
	          << " " << leaf.width() << " " << leaf.camber() << " "
	          << leaf.twistDegrees() << " " << leaf.thickness() << " "
	          << leaf.minimumBranchOrder() << ") petiole("
	          << petiole.length() << " " << petiole.baseRadius() << " "
	          << petiole.tipRadius() << ")";
	if (compound_leaf.isEnabled()) {
		canonical << " leafArray(" << compound_leaf.leafletNodeCount() << " "
		          << compound_leaf.rachisLength() << " "
		          << compound_leaf.rachisBaseRadius() << " "
		          << compound_leaf.rachisTipRadius() << " "
		          << phyllotaxis_name(compound_leaf.pattern()) << " "
		          << compound_leaf.divergenceDegrees() << " "
		          << compound_leaf.orientationUpBias() << " "
		          << compound_leaf.leafletScale() << ")";
	}
	canonical << " phyllotaxis("
	          << phyllotaxis_name(phyllotaxis.mode()) << " "
	          << phyllotaxis.divergenceDegrees() << " "
	          << phyllotaxis.internodeLength() << " "
	          << phyllotaxis.organsPerNode() << " "
	          << phyllotaxis.phaseDegrees() << " "
	          << phyllotaxis.radialOffset() << " "
	          << phyllotaxis.orientationUpBias() << ") floweringAge("
	          << species.floweringAge() << ") matureAge(" << species.matureAge()
	          << ")";
	if (flower.isEnabled()) {
		canonical << " flower(" << botanicalBladeProfileName(flower.petalProfile())
		          << " " << flower.petalLength() << " " << flower.petalWidth()
		          << " " << flower.longitudinalCurvature() << " "
		          << flower.camber() << " " << flower.twistDegrees() << " "
		          << flower.thickness() << " " << flower.widthPower()
		          << ") whorl(" << flower.petalWhorl().organCount() << " "
		          << flower.petalWhorl().radius() << " "
		          << flower.petalWhorl().phaseDegrees() << " "
		          << flower.petalWhorl().tiltDegrees() << ")";
		if (flower.flowerHead().isEnabled()) {
			const FlowerHeadSpecification &flower_head = flower.flowerHead();
			canonical << " flowerHead(" << flower_head.floretCount() << " "
			          << flower_head.radius() << " "
			          << flower_head.divergenceDegrees() << " "
			          << flower_head.phaseDegrees() << " "
			          << flower_head.tiltDegrees() << " "
			          << flower_head.scaleFalloff() << ")";
		}
		if (flower.inflorescence().isEnabled()) {
			const InflorescenceSpecification &inflorescence =
				flower.inflorescence();
			canonical << " inflorescence("
			          << inflorescenceKindName(inflorescence.kind()) << " "
			          << inflorescence.flowerCount() << " "
			          << inflorescence.spacing() << " "
			          << inflorescence.radialExtent() << " "
			          << inflorescence.phaseDegrees() << " "
			          << inflorescence.tiltDegrees() << " "
			          << inflorescence.scaleFalloff() << ")";
		}
	}
	if (fruit.isEnabled()) {
		canonical << " fruit(" << fruit.height() << " "
		          << fruit.maximumRadius() << " "
		          << fruit.shoulderFraction() << " " << fruit.fullness()
		          << " " << fruit.radialSegments() << " "
		          << fruit.profileSegments() << ")";
	}
	for (const PlantOrganArraySpecification &organ_array :
	     species.organArrays()) {
		canonical << " organArray("
		          << vegetationOrganTypeName(organ_array.organType()) << " "
		          << vegetationOrganArrayHostName(organ_array.host()) << " "
		          << organ_array.count() << " " << organ_array.spacing() << " "
		          << organ_array.azimuthProgressionDegrees() << " "
		          << organ_array.initialScale() << " "
		          << organ_array.scaleFalloff() << " "
		          << vegetationOrganOrientationName(organ_array.orientation())
		          << " " << organ_array.jitterFraction() << " "
		          << organ_array.minimumBranchOrder() << ")";
	}
	if (topology_generation.method() ==
	    PlantTopologyGenerationMethod::LSystem) {
		const PlantLSystemSpecification &l_system =
			*topology_generation.lSystem();
		canonical << " lAxiom(";
		for (std::size_t index = 0u; index < l_system.axiom().size(); ++index) {
			if (index != 0u) canonical << " ";
			canonical << lSystemSymbolName(l_system.axiom()[index]);
		}
		canonical << ")";
		for (const LSystemProductionRule &rule : l_system.productionRules()) {
			canonical << " lRule(" << lSystemSymbolName(rule.predecessor());
			for (LSystemSymbol symbol : rule.successor()) {
				canonical << " " << lSystemSymbolName(symbol);
			}
			canonical << ")";
		}
		canonical << " lSystem(" << l_system.iterationCount() << " "
		          << l_system.turnAngleDegrees() << " "
		          << l_system.stepLength() << " " << l_system.baseRadius()
		          << " " << l_system.terminalRadius() << " "
		          << l_system.radiusConservationExponent() << ")";
	}
	if (topology_generation.method() ==
	    PlantTopologyGenerationMethod::SpaceColonization) {
		const PlantSpaceColonizationSpecification &space_colonization =
			*topology_generation.spaceColonization();
		append_crown_canonical(&canonical, space_colonization.crownVolume());
		const SpaceColonizationSpecification &colonization =
			space_colonization.colonization();
		canonical << " spaceColonization(" << colonization.influenceRadius()
		          << " " << colonization.killRadius() << " "
		          << colonization.stepLength() << " "
		          << colonization.maximumIterations() << " "
		          << colonization.attractionPointCount() << " "
		          << colonization.radiusDecay() << " "
		          << colonization.minimumRadius() << " "
		          << colonization.radiusConservationExponent() << ")";
		for (const VegetationObstacleBoundary &obstacle :
		     space_colonization.obstacles()) {
			canonical << " crownObstacle(" << obstacle.identifier() << " "
			          << obstacle.bounds().min.x << " " << obstacle.bounds().min.y
			          << " " << obstacle.bounds().min.z << " "
			          << obstacle.bounds().max.x << " " << obstacle.bounds().max.y
			          << " " << obstacle.bounds().max.z << " "
			          << obstacle.clearance() << ")";
		}
	}
	if (candidate.growth.has_value()) {
		const PlantGrowthSpecification &growth = *candidate.growth;
		canonical << " growth(" << growth.startTime() << " "
		          << growth.duration() << " "
		          << growth.lengthGrowth().initialFactor() << " "
		          << growthCurveName(growth.lengthGrowth().curve()) << " "
		          << growth.radiusGrowth().initialFactor() << " "
		          << growthCurveName(growth.radiusGrowth().curve()) << " "
		          << growth.organGrowth().initialFactor() << " "
		          << growthCurveName(growth.organGrowth().curve()) << ")";
	}
	for (const TropismInfluence &influence : candidate.tropism_influences) {
		canonical << " tropism(" << tropismTypeName(influence.type()) << " "
		          << influence.direction().x << " " << influence.direction().y
		          << " " << influence.direction().z << " "
		          << influence.weight() << ")";
	}
	canonical << " detail(" << geometryDetailLevelName(candidate.detail_level)
	          << "))";

	std::ostringstream key;
	key << "Plant:v1:species=" << species.identifier()
	    << ":architecture=" << static_cast<int>(species.architecture())
	    << ":age=" << float_bits(candidate.age)
	    << ":state=" << static_cast<int>(candidate.development_state)
	    << ":seed=" << candidate.deterministic_seed
	    << ":generation=" << static_cast<int>(topology_generation.method())
	    << ":axis=" << float_bits(branching.primaryAxisLength())
	    << ":base=" << float_bits(branching.baseRadius())
	    << ":tip=" << float_bits(branching.tipRadius())
	    << ":internodes=" << branching.primaryInternodeCount()
	    << ":order=" << branching.maximumBranchOrder()
	    << ":lateral=" << branching.lateralBranchesPerNode()
	    << ":every=" << branching.branchEveryNInternodes()
	    << ":angle=" << float_bits(branching.branchAngleDegrees())
	    << ":azimuth=" << float_bits(branching.azimuthDivergenceDegrees())
	    << ":fraction=" << float_bits(branching.branchLengthFraction())
	    << ":falloff=" << float_bits(branching.branchLengthFalloff())
	    << ":radius=" << float_bits(branching.continuationRadiusRatio())
	    << ":gamma=" << float_bits(branching.radiusConservationGamma())
	    << ":directionVariation=" << float_bits(branching.directionVariationDegrees())
	    << ":lengthVariation=" << float_bits(branching.lengthVariationFraction())
	    << ":tropism=" << float_bits(branching.upwardTropismWeight())
	    << ":leafProfile=" << static_cast<int>(leaf.profile())
	    << ":leafLength=" << float_bits(leaf.length())
	    << ":leafWidth=" << float_bits(leaf.width())
	    << ":leafCamber=" << float_bits(leaf.camber())
	    << ":leafTwist=" << float_bits(leaf.twistDegrees())
	    << ":leafThickness=" << float_bits(leaf.thickness())
	    << ":leafOrder=" << leaf.minimumBranchOrder()
	    << ":petioleLength=" << float_bits(petiole.length())
	    << ":petioleBaseRadius=" << float_bits(petiole.baseRadius())
	    << ":petioleTipRadius=" << float_bits(petiole.tipRadius())
	    << ":compoundLeafEnabled=" << compound_leaf.isEnabled()
	    << ":phyllotaxis=" << static_cast<int>(phyllotaxis.mode())
	    << ":divergence=" << float_bits(phyllotaxis.divergenceDegrees())
	    << ":spacing=" << float_bits(phyllotaxis.internodeLength())
	    << ":organs=" << phyllotaxis.organsPerNode()
	    << ":phase=" << float_bits(phyllotaxis.phaseDegrees())
	    << ":offset=" << float_bits(phyllotaxis.radialOffset())
	    << ":up=" << float_bits(phyllotaxis.orientationUpBias())
	    << ":flowering=" << float_bits(species.floweringAge())
	    << ":mature=" << float_bits(species.matureAge())
	    << ":flowerEnabled=" << flower.isEnabled();
	if (compound_leaf.isEnabled()) {
		key << ":leafletNodes=" << compound_leaf.leafletNodeCount()
		    << ":rachisLength=" << float_bits(compound_leaf.rachisLength())
		    << ":rachisBaseRadius="
		    << float_bits(compound_leaf.rachisBaseRadius())
		    << ":rachisTipRadius="
		    << float_bits(compound_leaf.rachisTipRadius())
		    << ":leafletPattern=" << static_cast<int>(compound_leaf.pattern())
		    << ":leafletDivergence="
		    << float_bits(compound_leaf.divergenceDegrees())
		    << ":leafletUpBias="
		    << float_bits(compound_leaf.orientationUpBias())
		    << ":leafletScale=" << float_bits(compound_leaf.leafletScale());
	}
	if (flower.isEnabled()) {
		key << ":petalProfile=" << static_cast<int>(flower.petalProfile())
		    << ":petalLength=" << float_bits(flower.petalLength())
		    << ":petalWidth=" << float_bits(flower.petalWidth())
		    << ":petalCurvature=" << float_bits(flower.longitudinalCurvature())
		    << ":petalCamber=" << float_bits(flower.camber())
		    << ":petalTwist=" << float_bits(flower.twistDegrees())
		    << ":petalThickness=" << float_bits(flower.thickness())
		    << ":petalWidthPower=" << float_bits(flower.widthPower())
		    << ":whorlCount=" << flower.petalWhorl().organCount()
		    << ":whorlRadius=" << float_bits(flower.petalWhorl().radius())
		    << ":whorlPhase=" << float_bits(flower.petalWhorl().phaseDegrees())
		    << ":whorlTilt=" << float_bits(flower.petalWhorl().tiltDegrees())
		    << ":flowerHeadEnabled=" << flower.flowerHead().isEnabled()
		    << ":inflorescenceEnabled=" << flower.inflorescence().isEnabled();
		if (flower.flowerHead().isEnabled()) {
			const FlowerHeadSpecification &flower_head = flower.flowerHead();
			key << ":flowerHeadCount=" << flower_head.floretCount()
			    << ":flowerHeadRadius=" << float_bits(flower_head.radius())
			    << ":flowerHeadDivergence="
			    << float_bits(flower_head.divergenceDegrees())
			    << ":flowerHeadPhase=" << float_bits(flower_head.phaseDegrees())
			    << ":flowerHeadTilt=" << float_bits(flower_head.tiltDegrees())
			    << ":flowerHeadFalloff=" << float_bits(flower_head.scaleFalloff());
		}
		if (flower.inflorescence().isEnabled()) {
			const InflorescenceSpecification &inflorescence =
				flower.inflorescence();
			key << ":inflorescenceKind="
			    << static_cast<int>(inflorescence.kind())
			    << ":inflorescenceCount=" << inflorescence.flowerCount()
			    << ":inflorescenceSpacing="
			    << float_bits(inflorescence.spacing())
			    << ":inflorescenceExtent="
			    << float_bits(inflorescence.radialExtent())
			    << ":inflorescencePhase="
			    << float_bits(inflorescence.phaseDegrees())
			    << ":inflorescenceTilt="
			    << float_bits(inflorescence.tiltDegrees())
			    << ":inflorescenceFalloff="
			    << float_bits(inflorescence.scaleFalloff());
		}
	}
	if (fruit.isEnabled()) {
		key << ":fruitHeight=" << float_bits(fruit.height())
		    << ":fruitRadius=" << float_bits(fruit.maximumRadius())
		    << ":fruitShoulder=" << float_bits(fruit.shoulderFraction())
		    << ":fruitFullness=" << float_bits(fruit.fullness())
		    << ":fruitRadialSegments=" << fruit.radialSegments()
		    << ":fruitProfileSegments=" << fruit.profileSegments();
	}
	for (const PlantOrganArraySpecification &organ_array :
	     species.organArrays()) {
		key << ":organArray=" << organ_array.identifier()
		    << "," << static_cast<int>(organ_array.organType())
		    << "," << static_cast<int>(organ_array.host())
		    << "," << organ_array.count()
		    << "," << float_bits(organ_array.spacing())
		    << "," << float_bits(organ_array.azimuthProgressionDegrees())
		    << "," << float_bits(organ_array.initialScale())
		    << "," << float_bits(organ_array.scaleFalloff())
		    << "," << static_cast<int>(organ_array.orientation())
		    << "," << float_bits(organ_array.jitterFraction())
			    << "," << organ_array.minimumBranchOrder();
	}
	if (topology_generation.method() ==
	    PlantTopologyGenerationMethod::LSystem) {
		const PlantLSystemSpecification &l_system =
			*topology_generation.lSystem();
		key << ":lSystemIterations=" << l_system.iterationCount()
		    << ":lSystemAngle=" << float_bits(l_system.turnAngleDegrees())
		    << ":lSystemStep=" << float_bits(l_system.stepLength())
		    << ":lSystemBase=" << float_bits(l_system.baseRadius())
		    << ":lSystemTerminal=" << float_bits(l_system.terminalRadius())
		    << ":lSystemGamma="
		    << float_bits(l_system.radiusConservationExponent())
		    << ":lSystemAxiom=";
		for (LSystemSymbol symbol : l_system.axiom()) {
			key << static_cast<int>(symbol) << ",";
		}
		for (const LSystemProductionRule &rule : l_system.productionRules()) {
			key << ":lSystemRule=" << static_cast<int>(rule.predecessor()) << ",";
			for (LSystemSymbol symbol : rule.successor()) {
				key << static_cast<int>(symbol) << ",";
			}
		}
	}
	if (topology_generation.method() ==
	    PlantTopologyGenerationMethod::SpaceColonization) {
		const PlantSpaceColonizationSpecification &space_colonization =
			*topology_generation.spaceColonization();
		append_crown_key(&key, space_colonization.crownVolume());
		const SpaceColonizationSpecification &colonization =
			space_colonization.colonization();
		key << ":colonizationInfluence=" << float_bits(colonization.influenceRadius())
		    << ":colonizationKill=" << float_bits(colonization.killRadius())
		    << ":colonizationStep=" << float_bits(colonization.stepLength())
		    << ":colonizationIterations=" << colonization.maximumIterations()
		    << ":colonizationAttractions=" << colonization.attractionPointCount()
		    << ":colonizationRadiusDecay=" << float_bits(colonization.radiusDecay())
		    << ":colonizationMinimumRadius=" << float_bits(colonization.minimumRadius())
		    << ":colonizationGamma="
		    << float_bits(colonization.radiusConservationExponent())
		    << ":colonizationTropism="
		    << float_bits(colonization.tropismDirection().x) << ","
		    << float_bits(colonization.tropismDirection().y) << ","
		    << float_bits(colonization.tropismDirection().z) << ","
		    << float_bits(colonization.tropismWeight());
		for (const VegetationObstacleBoundary &obstacle :
		     space_colonization.obstacles()) {
			key << ":crownObstacle=" << obstacle.identifier() << ","
			    << float_bits(obstacle.bounds().min.x) << ","
			    << float_bits(obstacle.bounds().min.y) << ","
			    << float_bits(obstacle.bounds().min.z) << ","
			    << float_bits(obstacle.bounds().max.x) << ","
			    << float_bits(obstacle.bounds().max.y) << ","
			    << float_bits(obstacle.bounds().max.z) << ","
			    << float_bits(obstacle.clearance());
		}
	}
	key << ":growthEnabled=" << candidate.growth.has_value();
	if (candidate.growth.has_value()) {
		const PlantGrowthSpecification &growth = *candidate.growth;
		key << ":growthStart=" << float_bits(growth.startTime())
		    << ":growthDuration=" << float_bits(growth.duration())
		    << ":lengthInitial=" << float_bits(growth.lengthGrowth().initialFactor())
		    << ":lengthCurve=" << static_cast<int>(growth.lengthGrowth().curve())
		    << ":radiusInitial=" << float_bits(growth.radiusGrowth().initialFactor())
		    << ":radiusCurve=" << static_cast<int>(growth.radiusGrowth().curve())
		    << ":organInitial=" << float_bits(growth.organGrowth().initialFactor())
		    << ":organCurve=" << static_cast<int>(growth.organGrowth().curve());
	}
	for (const TropismInfluence &influence : candidate.tropism_influences) {
		key << ":tropismType=" << static_cast<int>(influence.type())
		    << ":tropismX=" << float_bits(influence.direction().x)
		    << ":tropismY=" << float_bits(influence.direction().y)
		    << ":tropismZ=" << float_bits(influence.direction().z)
		    << ":tropismWeight=" << float_bits(influence.weight());
	}

	return std::make_shared<const PlantShapeSpecification>(
			std::move(*candidate.species), candidate.age,
			candidate.development_state, candidate.deterministic_seed,
			std::move(candidate.topology_generation),
			std::move(candidate.growth), std::move(candidate.tropism_influences),
		ShapeSpecificationKey(key.str()), canonical.str(), candidate.detail_level);
}
