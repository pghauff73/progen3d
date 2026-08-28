#include "grammar/service/ShapeSpecificationParser.h"

#include "grammar/model/AxialProfileDescriptorSyntax.h"
#include "grammar/model/BotanicalBladeShapeDescriptorSyntax.h"
#include "grammar/model/BranchJunctionShapeDescriptorSyntax.h"
#include "grammar/model/CylinderShapeDescriptorSyntax.h"
#include "grammar/model/CompoundShapeDescriptorSyntax.h"
#include "grammar/model/ExtrudeProfileDescriptorSyntax.h"
#include "grammar/model/InstanceArrayDescriptorSyntax.h"
#include "grammar/model/NestedSourceShapeDescriptorSyntax.h"
#include "grammar/model/PlantShapeDescriptorSyntax.h"
#include "grammar/model/ScatterRegionShapeDescriptorSyntax.h"
#include "grammar/model/ShapeAliasDescriptorSyntax.h"
#include "grammar/model/SphereShapeDescriptorSyntax.h"
#include "grammar/model/TaperedSweepShapeDescriptorSyntax.h"
#include "grammar/model/VineShapeDescriptorSyntax.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

struct OptionArity
{
	std::size_t minimum = 0;
	std::size_t maximum = 0;
};

const std::unordered_map<std::string, OptionArity> &cylinder_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"radial", {2, 2}},
		{"wall", {1, 1}},
		{"axial", {2, 2}},
		{"azimuth", {2, 2}},
		{"chord", {3, 3}},
		{"clip", {5, 5}},
		{"topology", {1, 1}},
		{"close", {1, 4}},
		{"segments", {1, 2}},
		{"mapping", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &sphere_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"radial", {2, 2}},
		{"wall", {1, 1}},
		{"polar", {2, 2}},
		{"azimuth", {2, 2}},
		{"clip", {5, 5}},
		{"slab", {5, 5}},
		{"topology", {1, 1}},
		{"close", {1, 4}},
		{"segments", {1, 2}},
		{"mapping", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &tapered_sweep_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"samples", {8, 8192}},
		{"up", {3, 3}},
		{"segments", {1, 1}},
		{"cap", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &branch_junction_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"core", {2, 2}},
		{"parent", {5, 5}},
		{"child", {5, 5}},
		{"segments", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &sweep_profile_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"profile", {3, 4097}},
		{"path", {6, 24576}},
		{"up", {3, 3}},
		{"cap", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &
variable_section_sweep_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"path", {6, 24576}},
		{"station", {3, 4104}},
		{"stationTransform", {6, 6}},
		{"nominalSection", {2, 4097}},
		{"up", {3, 3}},
		{"frame", {1, 1}},
		{"cap", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &sweep_disk_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"path", {6, 24576}},
		{"curve", {7, 12289}},
		{"up", {3, 3}},
		{"radius", {1, 1}},
		{"radiusStart", {1, 1}},
		{"radiusEnd", {1, 1}},
		{"longitudinalSegments", {1, 1}},
		{"radialSegments", {1, 1}},
		{"segments", {1, 1}},
		{"cap", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &revolve_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"profile", {3, 4097}},
		{"angle", {2, 2}},
		{"segments", {1, 1}},
		{"cap", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &loft_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"section", {4, 4098}},
		{"cap", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &surface_loft_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"section", {4, 4098}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &
curve_network_surface_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"uCurve", {7, 12289}},
		{"vCurve", {7, 12289}},
		{"samplesU", {1, 1}},
		{"samplesV", {1, 1}},
		{"tolerance", {1, 1}},
		{"method", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &shell_loft_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"section", {14, 8194}},
		{"cap", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &folded_profile_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"path", {4, 8192}},
		{"thickness", {1, 1}},
		{"depth", {1, 1}},
		{"cap", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &curved_panel_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"width", {1, 1}},
		{"height", {1, 1}},
		{"curvature", {1, 2}},
		{"thickness", {1, 1}},
		{"segments", {1, 2}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &formed_panel_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"section", {4, 4098}},
		{"thickness", {1, 1}},
		{"side", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &hosted_opening_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"host", {2, 4097}},
		{"opening", {2, 4097}},
		{"depth", {1, 1}},
		{"cap", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &embossed_bead_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"path", {6, 24576}},
		{"up", {3, 3}},
		{"width", {1, 1}},
		{"depth", {1, 1}},
		{"shoulder", {1, 1}},
		{"side", {1, 1}},
		{"end", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &edge_flange_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"path", {6, 24576}},
		{"up", {3, 3}},
		{"width", {1, 1}},
		{"thickness", {1, 1}},
		{"angle", {1, 1}},
		{"bend", {1, 1}},
		{"side", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &panel_cut_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"path", {6, 24576}},
		{"gap", {1, 2}},
		{"up", {3, 3}},
		{"cap", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &mirror_shape_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"plane", {2, 2}},
		{"mode", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &shell_offset_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"thickness", {1, 1}},
		{"side", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &vehicle_body_shell_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"thickness", {1, 1}},
		{"section", {7, 7}},
		{"cap", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &generated_mesh_reference_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"meshKey", {1, 1}},
		{"detail", {1, 1}},
		{"topology", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &botanical_blade_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"profile", {1, 1}},
		{"length", {1, 1}},
		{"width", {1, 1}},
		{"curvature", {1, 1}},
		{"camber", {1, 1}},
		{"twist", {1, 1}},
		{"thickness", {1, 1}},
		{"widthPower", {1, 1}},
		{"segments", {2, 2}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &plant_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"species", {1, 1}},
		{"architecture", {1, 1}},
		{"age", {1, 1}},
		{"state", {1, 1}},
		{"seed", {1, 1}},
		{"generation", {1, 1}},
		{"lAxiom", {1, 128}},
		{"lRule", {2, 128}},
		{"lSystem", {6, 6}},
		{"trunk", {4, 4}},
		{"branching", {12, 12}},
		{"leaf", {7, 7}},
		{"petiole", {3, 3}},
		{"leafArray", {8, 8}},
		{"phyllotaxis", {3, 7}},
		{"organArray", {10, 10}},
		{"flower", {8, 8}},
		{"flowerHead", {6, 6}},
		{"inflorescence", {7, 7}},
		{"fruit", {6, 6}},
		{"whorl", {4, 4}},
		{"growth", {8, 8}},
		{"tropism", {5, 5}},
		{"crown", {5, 9}},
		{"crownCustom", {1, 1}},
		{"crownSample", {3, 3}},
		{"spaceColonization", {8, 8}},
		{"crownObstacle", {8, 8}},
		{"floweringAge", {1, 1}},
		{"matureAge", {1, 1}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &vine_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"species", {1, 1}},
		{"start", {3, 3}},
		{"direction", {3, 3}},
		{"radius", {1, 1}},
		{"mode", {1, 1}},
		{"collision", {1, 1}},
		{"attachment", {1, 1}},
		{"step", {1, 1}},
		{"segments", {1, 1}},
		{"seekDistance", {1, 1}},
		{"attachDistance", {1, 1}},
		{"tolerance", {1, 1}},
		{"radiusDecay", {1, 1}},
		{"minimumRadius", {1, 1}},
		{"preferred", {3, 3}},
		{"gamma", {1, 1}},
		{"target", {7, 7}},
		{"obstacle", {8, 8}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_map<std::string, OptionArity> &scatter_region_option_arities()
{
	static const std::unordered_map<std::string, OptionArity> arities = {
		{"region", {1, 1}},
		{"species", {1, 1}},
		{"seed", {1, 1}},
		{"surface", {7, 7}},
		{"face", {1, 1}},
		{"density", {1, 1}},
		{"separation", {1, 1}},
		{"scale", {2, 2}},
		{"orientation", {1, 1}},
		{"collisionRadius", {1, 1}},
		{"layer", {1, 1}},
		{"mask", {1, 10}},
		{"obstacle", {8, 8}},
		{"detail", {1, 1}},
	};
	return arities;
}

const std::unordered_set<std::string> &alias_names()
{
	static const std::unordered_set<std::string> names = {
		"Tube",
		"CylinderSector",
		"DSection",
		"HalfCylinder",
		"Hemisphere",
		"SphereCap",
		"SphereBowl",
		"SphereQuarter",
		"SphereOctant",
	};
	return names;
}

bool is_cylinder_alias(const std::string &shape_name)
{
	return shape_name == "Tube" || shape_name == "CylinderSector" ||
	       shape_name == "DSection" || shape_name == "HalfCylinder";
}

bool is_identifier(const std::string &text)
{
	if (text.empty() ||
	    !(std::isalpha(static_cast<unsigned char>(text.front())) ||
	      text.front() == '_')) {
		return false;
	}
	for (char character : text) {
		if (!(std::isalnum(static_cast<unsigned char>(character)) ||
		      character == '_')) {
			return false;
		}
	}
	return true;
}

bool validate_profile_constructor_arity(
	const std::string &constructor_name,
	std::size_t argument_count,
	std::string *diagnostic)
{
	bool valid = false;
	if (constructor_name == "Rect") valid = argument_count == 2u;
	else if (constructor_name == "RoundedRect") {
		valid = argument_count == 3u || argument_count == 4u;
	}
	else if (constructor_name == "Circle") {
		valid = argument_count == 1u || argument_count == 2u;
	}
	else if (constructor_name == "Ellipse") {
		valid = argument_count == 2u || argument_count == 3u;
	}
	else if (constructor_name == "ChamferRect") valid = argument_count == 3u;
	else if (constructor_name == "ThinWallBox" ||
	         constructor_name == "ThinWallChannel") {
		valid = argument_count == 4u;
	}
	else if (constructor_name == "ThinWallHat" ||
	         constructor_name == "ThinWallMultiCell") {
		valid = argument_count == 5u;
	}
	else if (constructor_name == "Polygon") {
		valid = argument_count >= 6u && argument_count % 2u == 0u;
	}
	if (!valid && diagnostic != nullptr) {
		*diagnostic = "Profile constructor '" + constructor_name +
		              "' has an unsupported argument count.";
	}
	return valid;
}

std::shared_ptr<const ShapeDescriptorSyntax> parse_extrude_profile_descriptor(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	const ShapeSpecificationParser::ExpressionArgumentParser &expression_parser,
	std::string *diagnostic)
{
	if (index == nullptr || *index >= raw_tokens.size() || raw_tokens[*index] != "(") {
		if (diagnostic != nullptr) *diagnostic = "Extrude requires an opening parenthesis.";
		return {};
	}
	++(*index);
	if (*index >= raw_tokens.size()) {
		if (diagnostic != nullptr) *diagnostic = "Extrude requires a profile constructor.";
		return {};
	}
	const std::string constructor_name = raw_tokens[*index];
	++(*index);
	if (constructor_name != "Rect" && constructor_name != "RoundedRect" &&
	    constructor_name != "Circle" && constructor_name != "Ellipse" &&
	    constructor_name != "ChamferRect" && constructor_name != "Polygon") {
		if (diagnostic != nullptr) {
			*diagnostic = "Extrude profile constructor '" + constructor_name +
			              "' is not supported.";
		}
		return {};
	}
	if (*index >= raw_tokens.size() || raw_tokens[*index] != "(") {
		if (diagnostic != nullptr) {
			*diagnostic = "Profile constructor '" + constructor_name +
			              "' requires an opening parenthesis.";
		}
		return {};
	}
	++(*index);
	std::vector<GeometryExpression> profile_arguments;
	try {
		while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
			profile_arguments.emplace_back(expression_parser(raw_tokens, index, ")"));
		}
	}
	catch (...) {
		if (diagnostic != nullptr) {
			*diagnostic = "Could not parse arguments for profile constructor '" +
			              constructor_name + "'.";
		}
		return {};
	}
	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) {
			*diagnostic = "Profile constructor '" + constructor_name +
			              "' is missing its closing parenthesis.";
		}
		return {};
	}
	++(*index);
	if (!validate_profile_constructor_arity(
			constructor_name, profile_arguments.size(), diagnostic)) {
		return {};
	}

	GeometryExpression depth("0");
	try {
		if (*index >= raw_tokens.size() || raw_tokens[*index] == ")" ||
		    raw_tokens[*index] == "cap") {
			if (diagnostic != nullptr) *diagnostic = "Extrude requires a depth expression.";
			return {};
		}
		depth = GeometryExpression(expression_parser(raw_tokens, index, ")"));
	}
	catch (...) {
		if (diagnostic != nullptr) *diagnostic = "Could not parse Extrude depth expression.";
		return {};
	}

	std::string cap_name = "all";
	if (*index < raw_tokens.size() && raw_tokens[*index] == "cap") {
		++(*index);
		if (*index >= raw_tokens.size() || raw_tokens[*index] != "(") {
			if (diagnostic != nullptr) *diagnostic = "Extrude cap requires an opening parenthesis.";
			return {};
		}
		++(*index);
		if (*index >= raw_tokens.size()) {
			if (diagnostic != nullptr) *diagnostic = "Extrude cap requires a policy name.";
			return {};
		}
		cap_name = raw_tokens[*index];
		++(*index);
		if (cap_name != "all" && cap_name != "none" && cap_name != "front" &&
		    cap_name != "back") {
			if (diagnostic != nullptr) {
				*diagnostic = "Extrude cap accepts all, none, front, or back.";
			}
			return {};
		}
		if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
			if (diagnostic != nullptr) *diagnostic = "Extrude cap is missing its closing parenthesis.";
			return {};
		}
		++(*index);
	}
	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) *diagnostic = "Extrude is missing its closing parenthesis.";
		return {};
	}
	++(*index);
	return std::make_shared<const ExtrudeProfileDescriptorSyntax>(
		Profile2DConstructorSyntax(constructor_name, std::move(profile_arguments)),
		std::move(depth),
		std::move(cap_name));
}

bool parse_axial_transform(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	const ShapeSpecificationParser::ExpressionArgumentParser &expression_parser,
	AxialProfileTransformSyntax *transform,
	std::string *diagnostic)
{
	GeometryExpression center_x("0");
	GeometryExpression center_z("0");
	GeometryExpression scale_x("1");
	GeometryExpression scale_z("1");
	GeometryExpression rotation("0");
	std::unordered_set<std::string> seen_options;
	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		const std::string option_name = raw_tokens[(*index)++];
		if (option_name != "center" && option_name != "scale" &&
		    option_name != "rotate") {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile level option '" + option_name +
				              "' is not supported.";
			}
			return false;
		}
		if (!seen_options.insert(option_name).second) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile level option '" + option_name +
				              "' may only be specified once.";
			}
			return false;
		}
		if (*index >= raw_tokens.size() || raw_tokens[*index] != "(") {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile level option '" + option_name +
				              "' requires an opening parenthesis.";
			}
			return false;
		}
		++(*index);
		std::vector<GeometryExpression> arguments;
		try {
			while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
				arguments.emplace_back(expression_parser(raw_tokens, index, ")"));
			}
		}
		catch (...) {
			if (diagnostic != nullptr) {
				*diagnostic = "Could not parse AxialProfile level option '" +
				              option_name + "'.";
			}
			return false;
		}
		if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile level option '" + option_name +
				              "' is missing its closing parenthesis.";
			}
			return false;
		}
		++(*index);
		const std::size_t required_arguments = option_name == "rotate" ? 1u : 2u;
		if (arguments.size() != required_arguments) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile level option '" + option_name +
				              "' requires " + std::to_string(required_arguments) +
				              " argument(s).";
			}
			return false;
		}
		if (option_name == "center") {
			center_x = std::move(arguments[0]);
			center_z = std::move(arguments[1]);
		} else if (option_name == "scale") {
			scale_x = std::move(arguments[0]);
			scale_z = std::move(arguments[1]);
		} else {
			rotation = std::move(arguments[0]);
		}
	}
	*transform = AxialProfileTransformSyntax(
		std::move(center_x), std::move(center_z),
		std::move(scale_x), std::move(scale_z), std::move(rotation));
	return true;
}

std::shared_ptr<const ShapeDescriptorSyntax> parse_axial_profile_descriptor(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	const ShapeSpecificationParser::ExpressionArgumentParser &expression_parser,
	std::string *diagnostic)
{
	++(*index);
	std::string axis_name;
	std::string cap_name = "all";
	std::vector<AxialProfilePolygonSyntax> profiles;
	std::vector<AxialProfileLevelSyntax> levels;
	std::unordered_set<std::string> profile_names;
	bool axis_seen = false;
	bool initial_seen = false;
	bool cap_seen = false;

	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		if (cap_seen) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile cap() must be the final statement.";
			}
			return {};
		}
		const std::string statement_name = raw_tokens[(*index)++];
		if (*index >= raw_tokens.size() || raw_tokens[*index] != "(") {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile statement '" + statement_name +
				              "' requires an opening parenthesis.";
			}
			return {};
		}
		++(*index);

		if (statement_name == "axis") {
			if (axis_seen || !profiles.empty() || initial_seen) {
				if (diagnostic != nullptr) {
					*diagnostic = "AxialProfile axis() must appear exactly once before profiles.";
				}
				return {};
			}
			if (*index >= raw_tokens.size() || !is_identifier(raw_tokens[*index])) {
				if (diagnostic != nullptr) *diagnostic = "AxialProfile axis() requires an axis name.";
				return {};
			}
			axis_name = raw_tokens[(*index)++];
			axis_seen = true;
		}
		else if (statement_name == "profile") {
			if (!axis_seen || initial_seen) {
				if (diagnostic != nullptr) {
					*diagnostic = "AxialProfile profiles must follow axis() and precede at().";
				}
				return {};
			}
			if (*index >= raw_tokens.size() || !is_identifier(raw_tokens[*index])) {
				if (diagnostic != nullptr) *diagnostic = "AxialProfile profile() requires a name.";
				return {};
			}
			const std::string profile_name = raw_tokens[(*index)++];
			if (!profile_names.insert(profile_name).second) {
				if (diagnostic != nullptr) {
					*diagnostic = "AxialProfile profile '" + profile_name +
					              "' is defined more than once.";
				}
				return {};
			}
			if (*index >= raw_tokens.size() || raw_tokens[*index] != "polygon") {
				if (diagnostic != nullptr) {
					*diagnostic = "AxialProfile profile '" + profile_name +
					              "' requires polygon(...).";
				}
				return {};
			}
			++(*index);
			if (*index >= raw_tokens.size() || raw_tokens[*index] != "(") {
				if (diagnostic != nullptr) *diagnostic = "AxialProfile polygon requires an opening parenthesis.";
				return {};
			}
			++(*index);
			std::vector<GeometryExpression> coordinates;
			try {
				while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
					coordinates.emplace_back(expression_parser(raw_tokens, index, ")"));
				}
			}
			catch (...) {
				if (diagnostic != nullptr) *diagnostic = "Could not parse AxialProfile polygon coordinates.";
				return {};
			}
			if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
				if (diagnostic != nullptr) *diagnostic = "AxialProfile polygon is missing its closing parenthesis.";
				return {};
			}
			++(*index);
			if (coordinates.size() < 6 || coordinates.size() % 2 != 0) {
				if (diagnostic != nullptr) {
					*diagnostic = "AxialProfile polygon() requires at least three X/Z coordinate pairs.";
				}
				return {};
			}
			profiles.emplace_back(profile_name, std::move(coordinates));
		}
		else if (statement_name == "at" || statement_name == "hold" ||
		         statement_name == "linear" || statement_name == "step") {
			if (!axis_seen || profiles.empty()) {
				if (diagnostic != nullptr) {
					*diagnostic = "AxialProfile transitions require declared profiles.";
				}
				return {};
			}
			const bool is_initial = statement_name == "at";
			if (is_initial == initial_seen && is_initial) {
				if (diagnostic != nullptr) *diagnostic = "AxialProfile may contain only one at() statement.";
				return {};
			}
			if (!is_initial && !initial_seen) {
				if (diagnostic != nullptr) *diagnostic = "AxialProfile transitions must follow at().";
				return {};
			}

			GeometryExpression axial_position(statement_name == "step" ? "" : "0");
			std::string profile_name;
			AxialProfileTransformSyntax transform;
			try {
				if (statement_name != "step") {
					axial_position = GeometryExpression(
						expression_parser(raw_tokens, index, ")"));
				}
			}
			catch (...) {
				if (diagnostic != nullptr) *diagnostic = "Could not parse AxialProfile axial position.";
				return {};
			}
			if (statement_name == "hold") {
				if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
					if (diagnostic != nullptr) *diagnostic = "AxialProfile hold() accepts only one axial expression.";
					return {};
				}
				levels.emplace_back(AxialTransitionKind::Hold,
				                    std::move(axial_position), "",
				                    AxialProfileTransformSyntax());
			} else {
				if (*index >= raw_tokens.size() || !is_identifier(raw_tokens[*index])) {
					if (diagnostic != nullptr) *diagnostic = "AxialProfile transition requires a profile name.";
					return {};
				}
				profile_name = raw_tokens[(*index)++];
				if (profile_names.count(profile_name) == 0) {
					if (diagnostic != nullptr) {
						*diagnostic = "AxialProfile profile '" + profile_name +
						              "' is not defined.";
					}
					return {};
				}
				if (!parse_axial_transform(raw_tokens, index, expression_parser,
				                           &transform, diagnostic)) {
					return {};
				}
				const AxialTransitionKind transition = statement_name == "at"
					? AxialTransitionKind::Initial
					: statement_name == "linear"
						? AxialTransitionKind::Linear
						: AxialTransitionKind::Step;
				levels.emplace_back(transition, std::move(axial_position),
				                    profile_name, std::move(transform));
				if (is_initial) initial_seen = true;
			}
		}
		else if (statement_name == "cap") {
			if (!initial_seen || cap_seen || *index >= raw_tokens.size()) {
				if (diagnostic != nullptr) *diagnostic = "AxialProfile cap() must follow its transitions.";
				return {};
			}
			cap_name = raw_tokens[(*index)++];
			if (cap_name != "none" && cap_name != "all" &&
			    cap_name != "bottom" && cap_name != "top") {
				if (diagnostic != nullptr) *diagnostic = "AxialProfile cap() accepts none, all, bottom, or top.";
				return {};
			}
			cap_seen = true;
		}
		else {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile statement '" + statement_name +
				              "' is not supported.";
			}
			return {};
		}

		if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile statement '" + statement_name +
				              "' is missing its closing parenthesis.";
			}
			return {};
		}
		++(*index);
	}
	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) *diagnostic = "AxialProfile is missing its closing parenthesis.";
		return {};
	}
	++(*index);
	if (!axis_seen || profiles.empty() || !initial_seen) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfile requires axis(y), profiles, and one initial at().";
		}
		return {};
	}
	return std::make_shared<const AxialProfileDescriptorSyntax>(
		axis_name, std::move(profiles), std::move(levels), cap_name);
}

std::shared_ptr<const ShapeDescriptorSyntax> parse_instance_array_descriptor(
	const ShapeSpecificationParser &parser,
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	const ShapeSpecificationParser::ExpressionArgumentParser &expression_parser,
	std::string *diagnostic)
{
	if (index == nullptr || *index >= raw_tokens.size() || raw_tokens[*index] != "(") {
		if (diagnostic != nullptr) *diagnostic = "InstanceArray requires an opening parenthesis.";
		return {};
	}
	++(*index);
	std::shared_ptr<const ShapeDescriptorSyntax> source_descriptor;
	InstanceArrayPatternSyntax pattern(InstanceArrayPatternKind::Linear, {});
	GeometryExpression detail("LOD5");
	bool pattern_seen = false;
	bool detail_seen = false;
	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		const std::string statement_name = raw_tokens[(*index)++];
		if (*index >= raw_tokens.size() || raw_tokens[*index] != "(") {
			if (diagnostic != nullptr) {
				*diagnostic = "InstanceArray statement '" + statement_name +
				              "' requires an opening parenthesis.";
			}
			return {};
		}
		++(*index);

		if (statement_name == "source") {
			if (source_descriptor != nullptr || *index >= raw_tokens.size() ||
			    !is_identifier(raw_tokens[*index])) {
				if (diagnostic != nullptr) {
					*diagnostic = "InstanceArray requires exactly one named source shape.";
				}
				return {};
			}
			const std::string source_name = raw_tokens[(*index)++];
			if (*index < raw_tokens.size() && raw_tokens[*index] == "(") {
				source_descriptor = parser.parseNestedDescriptor(
					source_name, raw_tokens, index, expression_parser, diagnostic);
			}
			else {
				source_descriptor = parser.createBareDescriptor(source_name, diagnostic);
			}
			if (!source_descriptor) return {};
		}
		else if (statement_name == "linear" || statement_name == "grid" ||
		         statement_name == "radial") {
			if (pattern_seen) {
				if (diagnostic != nullptr) {
					*diagnostic = "InstanceArray accepts exactly one linear, grid, or radial pattern.";
				}
				return {};
			}
			std::vector<GeometryExpression> arguments;
			try {
				while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
					arguments.emplace_back(expression_parser(raw_tokens, index, ")"));
				}
			}
			catch (...) {
				if (diagnostic != nullptr) *diagnostic = "Could not parse InstanceArray pattern arguments.";
				return {};
			}
			const std::size_t required_arguments = statement_name == "radial" ? 6u : 4u;
			if (arguments.size() != required_arguments) {
				if (diagnostic != nullptr) {
					*diagnostic = "InstanceArray " + statement_name + " requires " +
					              std::to_string(required_arguments) + " arguments.";
				}
				return {};
			}
			const InstanceArrayPatternKind kind = statement_name == "linear"
				? InstanceArrayPatternKind::Linear
				: statement_name == "grid"
					? InstanceArrayPatternKind::Grid
					: InstanceArrayPatternKind::Radial;
			pattern = InstanceArrayPatternSyntax(kind, std::move(arguments));
			pattern_seen = true;
		}
		else if (statement_name == "detail") {
			if (detail_seen) {
				if (diagnostic != nullptr) *diagnostic = "InstanceArray detail may be specified once.";
				return {};
			}
			try {
				detail = GeometryExpression(expression_parser(raw_tokens, index, ")"));
			}
			catch (...) {
				if (diagnostic != nullptr) *diagnostic = "Could not parse InstanceArray detail.";
				return {};
			}
			detail_seen = true;
		}
		else {
			if (diagnostic != nullptr) {
				*diagnostic = "InstanceArray statement '" + statement_name +
				              "' is not supported.";
			}
			return {};
		}

		if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
			if (diagnostic != nullptr) {
				*diagnostic = "InstanceArray statement '" + statement_name +
				              "' is missing its closing parenthesis.";
			}
			return {};
		}
		++(*index);
	}
	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) *diagnostic = "InstanceArray is missing its closing parenthesis.";
		return {};
	}
	++(*index);
	if (!source_descriptor || !pattern_seen) {
		if (diagnostic != nullptr) {
			*diagnostic = "InstanceArray requires source(...) and one linear, grid, or radial pattern.";
		}
		return {};
	}
	return std::make_shared<const InstanceArrayDescriptorSyntax>(
		std::move(source_descriptor), std::move(pattern), std::move(detail));
}

std::shared_ptr<const ShapeDescriptorSyntax> parse_compound_shape_descriptor(
	const ShapeSpecificationParser &parser,
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	const ShapeSpecificationParser::ExpressionArgumentParser &expression_parser,
	std::string *diagnostic)
{
	++(*index);
	std::vector<CompoundShapePartSyntax> parts;
	GeometryExpression detail("LOD3");
	bool detail_seen = false;
	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		const std::string statement_name = raw_tokens[(*index)++];
		if (*index >= raw_tokens.size() || raw_tokens[*index] != "(") {
			if (diagnostic != nullptr) {
				*diagnostic = "CompoundShape statement '" + statement_name +
				              "' requires an opening parenthesis.";
			}
			return {};
		}
		++(*index);
		if (statement_name == "detail") {
			if (detail_seen) {
				if (diagnostic != nullptr) {
					*diagnostic = "CompoundShape detail may only be specified once.";
				}
				return {};
			}
			try {
				detail = GeometryExpression(expression_parser(raw_tokens, index, ")"));
			}
			catch (...) {
				if (diagnostic != nullptr) *diagnostic = "Could not parse CompoundShape detail.";
				return {};
			}
			detail_seen = true;
		}
		else if (statement_name == "part") {
			if (*index >= raw_tokens.size() || !is_identifier(raw_tokens[*index])) {
				if (diagnostic != nullptr) {
					*diagnostic = "CompoundShape part requires a purpose identifier.";
				}
				return {};
			}
			const std::string purpose = raw_tokens[(*index)++];
			if (*index >= raw_tokens.size() || raw_tokens[(*index)++] != "source" ||
			    *index >= raw_tokens.size() || raw_tokens[(*index)++] != "(" ||
			    *index >= raw_tokens.size() || !is_identifier(raw_tokens[*index])) {
				if (diagnostic != nullptr) {
					*diagnostic = "CompoundShape part requires source(ShapeDescriptor).";
				}
				return {};
			}
			const std::string source_name = raw_tokens[(*index)++];
			std::shared_ptr<const ShapeDescriptorSyntax> source_descriptor;
			if (*index < raw_tokens.size() && raw_tokens[*index] == "(") {
				source_descriptor = parser.parseNestedDescriptor(
					source_name, raw_tokens, index, expression_parser, diagnostic);
			}
			else {
				source_descriptor = parser.createBareDescriptor(source_name, diagnostic);
			}
			if (!source_descriptor || *index >= raw_tokens.size() || raw_tokens[*index] != ")") {
				if (diagnostic != nullptr && diagnostic->empty()) {
					*diagnostic = "CompoundShape part source is missing its closing parenthesis.";
				}
				return {};
			}
			++(*index);
			std::vector<GeometryExpression> transform_arguments;
			if (*index < raw_tokens.size() && raw_tokens[*index] == "transform") {
				++(*index);
				if (*index >= raw_tokens.size() || raw_tokens[(*index)++] != "(") {
					if (diagnostic != nullptr) {
						*diagnostic = "CompoundShape part transform requires an opening parenthesis.";
					}
					return {};
				}
				try {
					while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
						transform_arguments.emplace_back(
							expression_parser(raw_tokens, index, ")"));
					}
				}
				catch (...) {
					if (diagnostic != nullptr) {
						*diagnostic = "Could not parse CompoundShape part transform.";
					}
					return {};
				}
				if (*index >= raw_tokens.size() || raw_tokens[*index] != ")" ||
				    transform_arguments.size() != 16u) {
					if (diagnostic != nullptr) {
						*diagnostic = "CompoundShape transform requires sixteen matrix values.";
					}
					return {};
				}
				++(*index);
			}
			else {
				for (int column = 0; column < 4; ++column) {
					for (int row = 0; row < 4; ++row) {
						transform_arguments.emplace_back(column == row ? "1" : "0");
					}
				}
			}
			parts.emplace_back(
				purpose, std::move(source_descriptor), std::move(transform_arguments));
		}
		else {
			if (diagnostic != nullptr) {
				*diagnostic = "CompoundShape supports only part(...) and detail(...).";
			}
			return {};
		}
		if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
			if (diagnostic != nullptr) {
				*diagnostic = "CompoundShape statement '" + statement_name +
				              "' is missing its closing parenthesis.";
			}
			return {};
		}
		++(*index);
	}
	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")" || parts.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = parts.empty()
				? "CompoundShape requires at least one part(...)."
				: "CompoundShape is missing its closing parenthesis.";
		}
		return {};
	}
	++(*index);
	return std::make_shared<const CompoundShapeDescriptorSyntax>(
		std::move(parts), std::move(detail));
}

const std::unordered_map<std::string, OptionArity> &alias_specific_option_arities(
	const std::string &shape_name)
{
	static const std::unordered_map<std::string, OptionArity> no_options;
	static const std::unordered_map<std::string, OptionArity> tube_options = {
		{"inner", {1, 1}},
	};
	static const std::unordered_map<std::string, OptionArity> sector_options = {
		{"sweep", {1, 1}},
	};
	static const std::unordered_map<std::string, OptionArity> d_section_options = {
		{"angle", {1, 1}}, {"offset", {1, 1}}, {"side", {1, 1}},
	};
	static const std::unordered_map<std::string, OptionArity> half_cylinder_options = {
		{"angle", {1, 1}}, {"side", {1, 1}},
	};
	static const std::unordered_map<std::string, OptionArity> hemisphere_options = {
		{"axis", {1, 1}}, {"side", {1, 1}},
	};
	static const std::unordered_map<std::string, OptionArity> sphere_cap_options = {
		{"axis", {1, 1}}, {"offset", {1, 1}}, {"side", {1, 1}},
	};
	static const std::unordered_map<std::string, OptionArity> sphere_bowl_options = {
		{"inner", {1, 1}}, {"axis", {1, 1}},
		{"offset", {1, 1}}, {"side", {1, 1}},
	};

	if (shape_name == "Tube") return tube_options;
	if (shape_name == "CylinderSector") return sector_options;
	if (shape_name == "DSection") return d_section_options;
	if (shape_name == "HalfCylinder") return half_cylinder_options;
	if (shape_name == "Hemisphere") return hemisphere_options;
	if (shape_name == "SphereCap") return sphere_cap_options;
	if (shape_name == "SphereBowl") return sphere_bowl_options;
	return no_options;
}

const OptionArity *find_option_arity(
	const std::string &shape_name,
	const std::string &option_name)
{
	if (shape_name == "SweepProfile") {
		const auto &arities = sweep_profile_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "VariableSectionSweep") {
		const auto &arities = variable_section_sweep_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "SweepDisk") {
		const auto &arities = sweep_disk_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "Revolve") {
		const auto &arities = revolve_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "Loft") {
		const auto &arities = loft_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "SurfaceLoft") {
		const auto &arities = surface_loft_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "CurveNetworkSurface") {
		const auto &arities = curve_network_surface_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "ShellLoft") {
		const auto &arities = shell_loft_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "FoldedProfile") {
		const auto &arities = folded_profile_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "CurvedPanel") {
		const auto &arities = curved_panel_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "FormedPanel") {
		const auto &arities = formed_panel_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "HostedOpening") {
		const auto &arities = hosted_opening_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "EmbossedBead") {
		const auto &arities = embossed_bead_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "EdgeFlange") {
		const auto &arities = edge_flange_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "PanelCut") {
		const auto &arities = panel_cut_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "MirrorShape") {
		const auto &arities = mirror_shape_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "ShellOffset") {
		const auto &arities = shell_offset_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "VehicleBodyShell") {
		const auto &arities = vehicle_body_shell_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "GeneratedMeshReference") {
		const auto &arities = generated_mesh_reference_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "TaperedSweep") {
		const auto &arities = tapered_sweep_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "BranchJunction") {
		const auto &arities = branch_junction_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "LeafBlade" || shape_name == "PetalBlade") {
		const auto &arities = botanical_blade_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "Plant") {
		const auto &arities = plant_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "Vine") {
		const auto &arities = vine_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (shape_name == "ScatterRegion") {
		const auto &arities = scatter_region_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}
	if (!ShapeSpecificationParser::isAliasShapeName(shape_name)) {
		const auto &arities = shape_name == "Cylinder"
			? cylinder_option_arities() : sphere_option_arities();
		const auto found = arities.find(option_name);
		return found == arities.end() ? nullptr : &found->second;
	}

	const auto &family_arities = is_cylinder_alias(shape_name)
		? cylinder_option_arities() : sphere_option_arities();
	const auto family_option = family_arities.find(option_name);
	if (family_option != family_arities.end()) return &family_option->second;
	const auto &alias_arities = alias_specific_option_arities(shape_name);
	const auto alias_option = alias_arities.find(option_name);
	return alias_option == alias_arities.end() ? nullptr : &alias_option->second;
}

std::shared_ptr<const ShapeDescriptorSyntax> parse_nested_source_descriptor(
	const ShapeSpecificationParser &parser,
	const std::string &shape_name,
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	const ShapeSpecificationParser::ExpressionArgumentParser &expression_parser,
	std::string *diagnostic)
{
	++(*index);
	std::shared_ptr<const ShapeDescriptorSyntax> source_descriptor;
	std::vector<ShapeOptionSyntax> options;
	std::unordered_set<std::string> seen_options;
	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		const std::string statement_name = raw_tokens[(*index)++];
		if (*index >= raw_tokens.size() || raw_tokens[*index] != "(") {
			if (diagnostic != nullptr) {
				*diagnostic = shape_name + " statement '" + statement_name +
				              "' requires an opening parenthesis.";
			}
			return {};
		}
		++(*index);
		if (statement_name == "source") {
			if (source_descriptor != nullptr || *index >= raw_tokens.size() ||
			    !is_identifier(raw_tokens[*index])) {
				if (diagnostic != nullptr) {
					*diagnostic = shape_name + " requires exactly one named source shape.";
				}
				return {};
			}
			const std::string source_name = raw_tokens[(*index)++];
			if (*index < raw_tokens.size() && raw_tokens[*index] == "(") {
				source_descriptor = parser.parseNestedDescriptor(
					source_name, raw_tokens, index, expression_parser, diagnostic);
			}
			else {
				source_descriptor = parser.createBareDescriptor(source_name, diagnostic);
			}
			if (!source_descriptor) return {};
		}
		else {
			std::vector<GeometryExpression> arguments;
			try {
				while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
					arguments.emplace_back(expression_parser(raw_tokens, index, ")"));
				}
			}
			catch (...) {
				if (diagnostic != nullptr) {
					*diagnostic = "Could not parse " + shape_name + " " +
					              statement_name + " arguments.";
				}
				return {};
			}
			const OptionArity *arity = find_option_arity(shape_name, statement_name);
			if (arity == nullptr || arguments.size() < arity->minimum ||
			    arguments.size() > arity->maximum) {
				if (diagnostic != nullptr) {
					*diagnostic = "Option '" + statement_name + "' for " +
					              shape_name + " has the wrong number of arguments.";
				}
				return {};
			}
			if (!seen_options.insert(statement_name).second) {
				if (diagnostic != nullptr) {
					*diagnostic = "Shape option '" + statement_name +
					              "' may only be specified once.";
				}
				return {};
			}
			options.emplace_back(statement_name, std::move(arguments));
		}

		if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
			if (diagnostic != nullptr) {
				*diagnostic = shape_name + " statement '" + statement_name +
				              "' is missing its closing parenthesis.";
			}
			return {};
		}
		++(*index);
	}
	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) {
			*diagnostic = shape_name + " is missing its closing parenthesis.";
		}
		return {};
	}
	++(*index);
	if (!source_descriptor) {
		if (diagnostic != nullptr) {
			*diagnostic = shape_name + " requires source(...).";
		}
		return {};
	}
	return std::make_shared<const NestedSourceShapeDescriptorSyntax>(
		shape_name, std::move(source_descriptor), std::move(options));
}

std::shared_ptr<const ShapeDescriptorSyntax> create_descriptor(
	const std::string &shape_name,
	std::vector<ShapeOptionSyntax> options)
{
	if (shape_name == "Cylinder") {
		return std::make_shared<const CylinderShapeDescriptorSyntax>(
			std::move(options));
	}
	if (shape_name == "Sphere") {
		return std::make_shared<const SphereShapeDescriptorSyntax>(
			std::move(options));
	}
	if (shape_name == "TaperedSweep") {
		return std::make_shared<const TaperedSweepShapeDescriptorSyntax>(
			std::move(options));
	}
	if (shape_name == "BranchJunction") {
		return std::make_shared<const BranchJunctionShapeDescriptorSyntax>(
			std::move(options));
	}
	if (shape_name == "LeafBlade") {
		return std::make_shared<const LeafBladeShapeDescriptorSyntax>(
			std::move(options));
	}
	if (shape_name == "PetalBlade") {
		return std::make_shared<const PetalBladeShapeDescriptorSyntax>(
			std::move(options));
	}
	if (shape_name == "Plant") {
		return std::make_shared<const PlantShapeDescriptorSyntax>(
			std::move(options));
	}
	if (shape_name == "Vine") {
		return std::make_shared<const VineShapeDescriptorSyntax>(
			std::move(options));
	}
	if (shape_name == "ScatterRegion") {
		return std::make_shared<const ScatterRegionShapeDescriptorSyntax>(
			std::move(options));
	}
	if (shape_name == "SweepProfile" || shape_name == "VariableSectionSweep" ||
	    shape_name == "SweepDisk" ||
	    shape_name == "Revolve" || shape_name == "Loft" ||
	    shape_name == "SurfaceLoft" || shape_name == "CurveNetworkSurface" ||
	    shape_name == "CurvedPanel" ||
	    shape_name == "FormedPanel" || shape_name == "HostedOpening" ||
	    shape_name == "EmbossedBead" || shape_name == "EdgeFlange" ||
	    shape_name == "PanelCut" ||
	    shape_name == "ShellLoft" || shape_name == "FoldedProfile" ||
	    shape_name == "VehicleBodyShell" ||
	    shape_name == "GeneratedMeshReference") {
		return std::make_shared<const StoredShapeDescriptorSyntax>(
			shape_name, std::move(options));
	}
	if (alias_names().count(shape_name) != 0) {
		return std::make_shared<const ShapeAliasDescriptorSyntax>(
			shape_name,
			std::move(options));
	}
	return {};
}

}

bool ShapeSpecificationParser::isSupportedShapeName(
	const std::string &shape_name)
{
	return isCanonicalShapeName(shape_name) || isAliasShapeName(shape_name);
}

bool ShapeSpecificationParser::isCanonicalShapeName(
	const std::string &shape_name)
{
	return shape_name == "Cylinder" || shape_name == "Sphere" ||
	       shape_name == "AxialProfile" || shape_name == "Extrude" ||
	       shape_name == "SweepProfile" || shape_name == "VariableSectionSweep" ||
	       shape_name == "SweepDisk" ||
	       shape_name == "Revolve" || shape_name == "Loft" ||
	       shape_name == "SurfaceLoft" || shape_name == "CurveNetworkSurface" ||
	       shape_name == "ShellOffset" ||
	       shape_name == "MirrorShape" || shape_name == "CurvedPanel" ||
	       shape_name == "FormedPanel" || shape_name == "HostedOpening" ||
	       shape_name == "EmbossedBead" || shape_name == "EdgeFlange" ||
	       shape_name == "PanelCut" ||
	       shape_name == "ShellLoft" || shape_name == "FoldedProfile" ||
	       shape_name == "InstanceArray" || shape_name == "CompoundShape" ||
	       shape_name == "VehicleBodyShell" ||
	       shape_name == "GeneratedMeshReference" ||
	       shape_name == "TaperedSweep" || shape_name == "BranchJunction" ||
	       shape_name == "LeafBlade" ||
	       shape_name == "PetalBlade" || shape_name == "Plant" ||
	       shape_name == "Vine" || shape_name == "ScatterRegion";
}

bool ShapeSpecificationParser::isAliasShapeName(
	const std::string &shape_name)
{
	return alias_names().count(shape_name) != 0;
}

std::shared_ptr<const ShapeDescriptorSyntax>
ShapeSpecificationParser::createBareDescriptor(
	const std::string &shape_name,
	std::string *diagnostic) const
{
	if (shape_name == "AxialProfile" || shape_name == "Extrude" ||
	    shape_name == "SweepProfile" || shape_name == "SweepDisk" ||
	    shape_name == "Revolve" || shape_name == "Loft" ||
	    shape_name == "SurfaceLoft" || shape_name == "CurveNetworkSurface" ||
	    shape_name == "ShellOffset" ||
	    shape_name == "MirrorShape" || shape_name == "CurvedPanel" ||
	    shape_name == "PanelCut" ||
	    shape_name == "ShellLoft" || shape_name == "FoldedProfile" ||
	    shape_name == "InstanceArray" ||
	    shape_name == "VehicleBodyShell" ||
	    shape_name == "GeneratedMeshReference" ||
	    shape_name == "TaperedSweep" || shape_name == "BranchJunction" ||
	    shape_name == "Plant" ||
	    shape_name == "Vine" || shape_name == "ScatterRegion") {
		if (diagnostic != nullptr) {
			*diagnostic = shape_name + " requires a nested descriptor.";
		}
		return {};
	}
	auto descriptor = create_descriptor(shape_name, {});
	if (!descriptor && diagnostic != nullptr) {
		*diagnostic = "Unknown shape family or alias '" + shape_name + "'.";
	}
	return descriptor;
}

std::shared_ptr<const ShapeDescriptorSyntax>
ShapeSpecificationParser::parseNestedDescriptor(
	const std::string &shape_name,
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	const ExpressionArgumentParser &expression_parser,
	std::string *diagnostic) const
{
	if (index == nullptr || *index >= raw_tokens.size() ||
	    raw_tokens[*index] != "(") {
		if (diagnostic != nullptr) {
			*diagnostic = "Shape descriptor '" + shape_name +
			              "' requires an opening parenthesis.";
		}
		return {};
	}
	if (!isSupportedShapeName(shape_name)) {
		if (diagnostic != nullptr) {
			*diagnostic = "Unknown shape family or alias '" + shape_name + "'.";
		}
		return {};
	}
	if (shape_name == "AxialProfile") {
		return parse_axial_profile_descriptor(
			raw_tokens, index, expression_parser, diagnostic);
	}
	if (shape_name == "Extrude") {
		return parse_extrude_profile_descriptor(
			raw_tokens, index, expression_parser, diagnostic);
	}
	if (shape_name == "InstanceArray") {
		return parse_instance_array_descriptor(
			*this, raw_tokens, index, expression_parser, diagnostic);
	}
	if (shape_name == "CompoundShape") {
		return parse_compound_shape_descriptor(
			*this, raw_tokens, index, expression_parser, diagnostic);
	}
	if (shape_name == "MirrorShape" || shape_name == "ShellOffset") {
		return parse_nested_source_descriptor(
			*this, shape_name, raw_tokens, index, expression_parser, diagnostic);
	}

	++(*index);
	std::vector<ShapeOptionSyntax> options;
	std::unordered_set<std::string> seen_options;
	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		const std::string option_name = raw_tokens[*index];
		++(*index);
		if (*index >= raw_tokens.size() || raw_tokens[*index] != "(") {
			if (diagnostic != nullptr) {
				*diagnostic = "Shape option '" + option_name +
				              "' requires an opening parenthesis.";
			}
			return {};
		}

		++(*index);
		std::vector<GeometryExpression> arguments;
		try {
			while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
				arguments.emplace_back(
					expression_parser(raw_tokens, index, ")"));
			}
		}
		catch (...) {
			if (diagnostic != nullptr) {
				*diagnostic = "Could not parse arguments for shape option '" +
				              option_name + "'.";
			}
			return {};
		}

		if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
			if (diagnostic != nullptr) {
				*diagnostic = "Shape option '" + option_name +
				              "' is missing its closing parenthesis.";
			}
			return {};
		}
		++(*index);

		ShapeOptionSyntax option(option_name, std::move(arguments));
		if (!validateOption(shape_name, option, diagnostic)) {
			return {};
		}
		if (option_name != "clip" && option_name != "obstacle" &&
		    option_name != "section" && option_name != "station" &&
		    option_name != "uCurve" && option_name != "vCurve" &&
		    option_name != "stationTransform" && option_name != "nominalSection" &&
		    option_name != "opening" &&
		    option_name != "tropism" &&
		    option_name != "child" && option_name != "lRule" &&
		    option_name != "crownSample" && option_name != "crownObstacle" &&
		    !seen_options.insert(option_name).second) {
			if (diagnostic != nullptr) {
				*diagnostic = "Shape option '" + option_name +
				              "' may only be specified once.";
			}
			return {};
		}
		options.push_back(std::move(option));
	}

	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) {
			*diagnostic = "Shape descriptor '" + shape_name +
			              "' is missing its closing parenthesis.";
		}
		return {};
	}
	++(*index);
	return create_descriptor(shape_name, std::move(options));
}

bool ShapeSpecificationParser::validateOption(
	const std::string &shape_name,
	const ShapeOptionSyntax &option,
	std::string *diagnostic) const
{
	const OptionArity *arity = find_option_arity(shape_name, option.optionName());
	if (arity == nullptr) {
		if (diagnostic != nullptr) {
			*diagnostic = "Option '" + option.optionName() +
			              "' is not supported by " + shape_name + ".";
		}
		return false;
	}

	const std::size_t count = option.arguments().size();
	if (shape_name == "TaperedSweep" && option.optionName() == "samples" &&
	    count % 4u != 0u) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"TaperedSweep samples() requires x y z radius groups.";
		}
		return false;
	}
	if ((shape_name == "PanelCut" && option.optionName() == "path") &&
	    count % 3u != 0u) {
		if (diagnostic != nullptr) {
			*diagnostic = "PanelCut path() requires x y z groups.";
		}
		return false;
	}
	if (count < arity->minimum || count > arity->maximum) {
		if (diagnostic != nullptr) {
			*diagnostic = "Option '" + option.optionName() + "' for " +
			              shape_name + " has the wrong number of arguments.";
		}
		return false;
	}
	return true;
}
