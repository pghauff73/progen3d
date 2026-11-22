#include "geometry/service/FacialSkinSpecificationValidator.h"

#include <cmath>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_set>

namespace {

bool finite_in_range(float value, float minimum, float maximum)
{
	return std::isfinite(value) && value >= minimum && value <= maximum;
}

bool valid_material_coordinate(const glm::vec2 &coordinate)
{
	return finite_in_range(coordinate.x, 0.0f, 1.0f) && finite_in_range(coordinate.y, 0.0f, 1.0f);
}

bool valid_patch_radii(const glm::vec2 &radii)
{
	return finite_in_range(radii.x, 0.01f, 0.5f) && finite_in_range(radii.y, 0.01f, 0.5f);
}

bool valid_declaration_name(const std::string &name)
{
	if (name.empty() || name.size() > 64) return false;
	const auto first = static_cast<unsigned char>(name.front());
	if (std::isalpha(first) == 0 && first != '_') return false;
	for (unsigned char character : name) {
		if (std::isalnum(character) == 0 && character != '_') return false;
	}
	return true;
}

bool declares_elastic_configuration(const FacialSkinShapeSpecificationCandidate &candidate)
{
	const FacialElasticMaterial default_material;
	const FacialElasticSolveSettings default_settings;
	return candidate.has_skin_material_option || candidate.has_solve_option ||
	       candidate.has_muscle_option || candidate.has_attachment_option || candidate.has_tissue_region_option ||
	       !candidate.muscles.empty() || !candidate.attachments.empty() || !candidate.tissue_regions.empty() ||
	       candidate.elastic_material.young_modulus_pa != default_material.young_modulus_pa ||
	       candidate.elastic_material.poisson_ratio != default_material.poisson_ratio ||
	       candidate.elastic_material.thickness_m != default_material.thickness_m ||
	       candidate.elastic_material.support_stiffness_n_per_m3 != default_material.support_stiffness_n_per_m3 ||
	       candidate.elastic_solve_settings.maximum_iterations != default_settings.maximum_iterations ||
	       candidate.elastic_solve_settings.relative_force_tolerance != default_settings.relative_force_tolerance;
}

bool validate_elastic_configuration(const FacialSkinShapeSpecificationCandidate &candidate,
	                               std::string *diagnostic)
{
	auto fail = [diagnostic](const std::string &message) {
		if (diagnostic != nullptr) *diagnostic = message;
		return false;
	};
	if (candidate.horizontal_segments > 96 || candidate.vertical_segments > 128) {
		return fail("FacialSkin elastic response supports at most segments(96 128); the analytic response retains higher detail levels.");
	}
	const FacialElasticMaterial &material = candidate.elastic_material;
	if (!finite_in_range(material.young_modulus_pa, 100.0f, 1.0e7f) ||
	    !finite_in_range(material.poisson_ratio, 0.0f, 0.49f) ||
	    !finite_in_range(material.thickness_m, 1.0e-5f, 0.02f) ||
	    !finite_in_range(material.support_stiffness_n_per_m3, 1.0f, 1.0e8f)) {
		return fail("FacialSkin skinMaterial requires Young's modulus [100,10000000] Pa, Poisson ratio [0,0.49], thickness [0.00001,0.02] m, and support [1,100000000] N/m^3.");
	}
	const FacialElasticSolveSettings &settings = candidate.elastic_solve_settings;
	if (settings.maximum_iterations < 8 || settings.maximum_iterations > 2000 ||
	    !finite_in_range(settings.relative_force_tolerance, 1.0e-7f, 1.0e-2f)) {
		return fail("FacialSkin solve requires integer iterations in [8,2000] and relative force tolerance in [0.0000001,0.01].");
	}
	if (candidate.muscles.size() > 16 || candidate.attachments.size() > 16 || candidate.tissue_regions.size() > 16) {
		return fail("FacialSkin supports at most 16 muscles, 16 attachments, and 16 tissue regions per elastic face.");
	}
	std::unordered_set<std::string> names;
	for (const FacialMuscleSpecification &muscle : candidate.muscles) {
		if (!valid_declaration_name(muscle.name) || !names.insert(muscle.name).second) {
			return fail("FacialSkin muscle names must be unique identifiers of 1 to 64 letters, digits, or underscores, starting with a letter or underscore: '" + muscle.name + "'.");
		}
		if (!valid_material_coordinate(muscle.origin_uv) || !valid_material_coordinate(muscle.insertion_uv) ||
		    !valid_patch_radii(muscle.influence_radius_uv) ||
		    !finite_in_range(muscle.activation, 0.0f, 1.0f) ||
		    !finite_in_range(muscle.maximum_shortening, 0.0f, 0.25f) ||
		    !finite_in_range(muscle.stiffness_n_per_m, 0.0f, 10000.0f)) {
			return fail("FacialSkin muscle '" + muscle.name + "' requires origin/insertion UV in [0,1], radii in [0.01,0.5], activation in [0,1], maximum shortening in [0,0.25], and stiffness in [0,10000] N/m.");
		}
		const glm::vec2 attachment_span = muscle.origin_uv - muscle.insertion_uv;
		if (glm::dot(attachment_span, attachment_span) <= 1.0e-12f) {
			return fail("FacialSkin muscle '" + muscle.name + "' requires distinct origin and insertion UV coordinates.");
		}
	}
	names.clear();
	for (const FacialAttachmentPatch &attachment : candidate.attachments) {
		if (!valid_declaration_name(attachment.name) || !names.insert(attachment.name).second) {
			return fail("FacialSkin attachment names must be unique identifiers of 1 to 64 letters, digits, or underscores, starting with a letter or underscore: '" + attachment.name + "'.");
		}
		if (!valid_material_coordinate(attachment.center_uv) || !valid_patch_radii(attachment.influence_radius_uv) ||
		    !finite_in_range(attachment.stiffness_n_per_m, 0.0f, 1.0e6f)) {
			return fail("FacialSkin attachment '" + attachment.name + "' requires UV in [0,1], radii in [0.01,0.5], and stiffness in [0,1000000] N/m.");
		}
	}
	names.clear();
	for (const FacialTissueRegion &region : candidate.tissue_regions) {
		if (!valid_declaration_name(region.name) || !names.insert(region.name).second) {
			return fail("FacialSkin tissueRegion names must be unique identifiers of 1 to 64 letters, digits, or underscores, starting with a letter or underscore: '" + region.name + "'.");
		}
		if (!valid_material_coordinate(region.center_uv) || !valid_patch_radii(region.influence_radius_uv) ||
		    !finite_in_range(region.young_modulus_scale, 0.1f, 10.0f) ||
		    !finite_in_range(region.support_stiffness_scale, 0.1f, 10.0f)) {
			return fail("FacialSkin tissueRegion '" + region.name + "' requires UV in [0,1], radii in [0.01,0.5], and Young/support scales in [0.1,10].");
		}
	}
	return true;
}

} // namespace

std::shared_ptr<const FacialSkinShapeSpecification>
FacialSkinSpecificationValidator::validate(
	const FacialSkinShapeSpecificationCandidate &candidate,
	std::string *diagnostic) const
{
	for (int axis = 0; axis < 3; ++axis) {
		if (!std::isfinite(candidate.size[axis]) || candidate.size[axis] <= 1.0e-5f ||
		    candidate.size[axis] > 1000.0f) {
			if (diagnostic != nullptr) {
				*diagnostic = "FacialSkin size must contain finite positive dimensions in (0.00001, 1000].";
			}
			return {};
		}
	}
	const float height_ratio = candidate.size.y / candidate.size.x;
	const float depth_ratio = candidate.size.z / candidate.size.x;
	if (height_ratio < 0.5f || height_ratio > 2.0f || depth_ratio < 0.05f || depth_ratio > 1.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "FacialSkin requires height/width in [0.5,2] and depth/width in [0.05,1]; use uniform scale for miniature or large heads.";
		}
		return {};
	}
	// These multiples keep all three aperture boundaries and semantic landmarks
	// at exactly the same rest coordinates at every supported detail level.
	if (candidate.horizontal_segments < 48 || candidate.horizontal_segments > 192 ||
	    candidate.horizontal_segments % 48 != 0 ||
	    candidate.vertical_segments < 64 || candidate.vertical_segments > 256 ||
	    candidate.vertical_segments % 64 != 0) {
		if (diagnostic != nullptr) {
			*diagnostic = "FacialSkin segments require horizontal multiples of 48 in [48,192] and vertical multiples of 64 in [64,256].";
		}
		return {};
	}
	const auto &controls = candidate.controls;
	const float values[] = {controls.smile_left, controls.smile_right,
		controls.jaw_open, controls.blink_left, controls.blink_right,
		controls.brow_raise_left, controls.brow_raise_right};
	for (float value : values) {
		if (!std::isfinite(value) || value < 0.0f || value > 1.0f) {
			if (diagnostic != nullptr) {
				*diagnostic = "FacialSkin smile, jawOpen, blink and browRaise controls must be finite values in [0,1].";
			}
			return {};
		}
	}
	if (candidate.response_mode != FacialSkinResponseMode::Analytic &&
	    candidate.response_mode != FacialSkinResponseMode::Elastic) {
		if (diagnostic != nullptr) *diagnostic = "FacialSkin response must be analytic or elastic.";
		return {};
	}
	const bool elastic_response = candidate.response_mode == FacialSkinResponseMode::Elastic;
	if (!elastic_response && declares_elastic_configuration(candidate)) {
		if (diagnostic != nullptr) {
			*diagnostic = "FacialSkin skinMaterial, solve, muscle, attachment and tissueRegion options require response(elastic); they are not applied to response(analytic).";
		}
		return {};
	}
	if (elastic_response) {
		for (float value : values) {
			if (value != 0.0f) {
				if (diagnostic != nullptr) {
					*diagnostic = "FacialSkin response(elastic) requires smile, jawOpen, blink and browRaise controls to be zero; use explicit muscle(...) activation for the neutral-reference elastic model.";
				}
				return {};
			}
		}
		if (!validate_elastic_configuration(candidate, diagnostic)) return {};
	}
	std::ostringstream key;
	key << (elastic_response ? "FacialSkin:v2:elastic:" : "FacialSkin:v1:") << std::hexfloat
	    << candidate.size.x << ',' << candidate.size.y << ',' << candidate.size.z
	    << ':' << candidate.horizontal_segments << ',' << candidate.vertical_segments;
	for (float value : values) key << ':' << value;
	std::ostringstream canonical;
	canonical << std::setprecision(std::numeric_limits<float>::max_digits10)
	          << "FacialSkin(size(" << candidate.size.x << ' ' << candidate.size.y
	          << ' ' << candidate.size.z << ") segments("
	          << candidate.horizontal_segments << ' ' << candidate.vertical_segments
	          << ") smile(" << controls.smile_left << ' ' << controls.smile_right
	          << ") jawOpen(" << controls.jaw_open << ") blink("
	          << controls.blink_left << ' ' << controls.blink_right
	          << ") browRaise(" << controls.brow_raise_left << ' '
	          << controls.brow_raise_right << ")";
	if (elastic_response) {
		const FacialElasticMaterial &material = candidate.elastic_material;
		const FacialElasticSolveSettings &settings = candidate.elastic_solve_settings;
		key << ":material:" << material.young_modulus_pa << ':' << material.poisson_ratio
		    << ':' << material.thickness_m << ':' << material.support_stiffness_n_per_m3
		    << ":solve:" << settings.maximum_iterations << ':' << settings.relative_force_tolerance;
		canonical << " response(elastic) skinMaterial(" << material.young_modulus_pa << ' '
		          << material.poisson_ratio << ' ' << material.thickness_m << ' ' << material.support_stiffness_n_per_m3
		          << ") solve(" << settings.maximum_iterations << ' ' << settings.relative_force_tolerance << ')';
		for (const FacialMuscleSpecification &muscle : candidate.muscles) {
			key << ":muscle:" << muscle.name << ':' << muscle.origin_uv.x << ':' << muscle.origin_uv.y
			    << ':' << muscle.insertion_uv.x << ':' << muscle.insertion_uv.y
			    << ':' << muscle.influence_radius_uv.x << ':' << muscle.influence_radius_uv.y
			    << ':' << muscle.activation << ':' << muscle.maximum_shortening << ':' << muscle.stiffness_n_per_m;
			canonical << " muscle(" << muscle.name << ' ' << muscle.origin_uv.x << ' ' << muscle.origin_uv.y
			          << ' ' << muscle.insertion_uv.x << ' ' << muscle.insertion_uv.y
			          << ' ' << muscle.influence_radius_uv.x << ' ' << muscle.influence_radius_uv.y
			          << ' ' << muscle.activation << ' ' << muscle.maximum_shortening << ' ' << muscle.stiffness_n_per_m << ')';
		}
		for (const FacialAttachmentPatch &attachment : candidate.attachments) {
			key << ":attachment:" << attachment.name << ':' << attachment.center_uv.x << ':' << attachment.center_uv.y
			    << ':' << attachment.influence_radius_uv.x << ':' << attachment.influence_radius_uv.y << ':' << attachment.stiffness_n_per_m;
			canonical << " attachment(" << attachment.name << ' ' << attachment.center_uv.x << ' ' << attachment.center_uv.y
			          << ' ' << attachment.influence_radius_uv.x << ' ' << attachment.influence_radius_uv.y << ' ' << attachment.stiffness_n_per_m << ')';
		}
		for (const FacialTissueRegion &region : candidate.tissue_regions) {
			key << ":tissueRegion:" << region.name << ':' << region.center_uv.x << ':' << region.center_uv.y
			    << ':' << region.influence_radius_uv.x << ':' << region.influence_radius_uv.y
			    << ':' << region.young_modulus_scale << ':' << region.support_stiffness_scale;
			canonical << " tissueRegion(" << region.name << ' ' << region.center_uv.x << ' ' << region.center_uv.y
			          << ' ' << region.influence_radius_uv.x << ' ' << region.influence_radius_uv.y
			          << ' ' << region.young_modulus_scale << ' ' << region.support_stiffness_scale << ')';
		}
	}
	canonical << ')';
	return std::make_shared<const FacialSkinShapeSpecification>(
		candidate.size, candidate.horizontal_segments, candidate.vertical_segments,
		controls, ShapeSpecificationKey(key.str()), canonical.str(), candidate.response_mode,
		candidate.elastic_material, candidate.elastic_solve_settings, candidate.muscles, candidate.attachments, candidate.tissue_regions);
}
