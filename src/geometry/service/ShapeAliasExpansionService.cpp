#include "geometry/service/ShapeAliasExpansionService.h"

#include "grammar/model/CylinderShapeDescriptorSyntax.h"
#include "grammar/model/GeometryExpression.h"
#include "grammar/model/SphereShapeDescriptorSyntax.h"
#include "grammar/service/ShapeSpecificationParser.h"

#include <algorithm>
#include <utility>

namespace {

ShapeOptionSyntax option(
	const std::string &name,
	std::initializer_list<const char *> argument_text)
{
	std::vector<GeometryExpression> arguments;
	for (const char *text : argument_text) {
		arguments.emplace_back(text);
	}
	return ShapeOptionSyntax(name, std::move(arguments));
}

bool contains_option(const std::vector<ShapeOptionSyntax> &options,
	                 const std::string &name)
{
	return std::any_of(options.begin(), options.end(),
		[&](const ShapeOptionSyntax &candidate) {
			return candidate.optionName() == name;
		});
}

const ShapeOptionSyntax *find_option(
	const std::vector<ShapeOptionSyntax> &options,
	const std::string &name)
{
	const auto found = std::find_if(options.begin(), options.end(),
		[&](const ShapeOptionSyntax &candidate) {
			return candidate.optionName() == name;
		});
	return found == options.end() ? nullptr : &*found;
}

void append_passthrough_options(
	const std::vector<ShapeOptionSyntax> &source,
	const std::vector<std::string> &excluded,
	std::vector<ShapeOptionSyntax> *target)
{
	for (const ShapeOptionSyntax &candidate : source) {
		if (std::find(excluded.begin(), excluded.end(), candidate.optionName()) ==
		    excluded.end()) {
			target->push_back(candidate);
		}
	}
}

std::vector<GeometryExpression> clip_arguments_for_axis(
	const ShapeOptionSyntax *axis_option,
	const ShapeOptionSyntax *offset_option,
	const ShapeOptionSyntax *side_option,
	const char *default_offset)
{
	std::string axis = "y";
	if (axis_option != nullptr && !axis_option->arguments().empty()) {
		axis = axis_option->arguments()[0].sourceText();
	}
	std::string side = "positive";
	if (side_option != nullptr && !side_option->arguments().empty()) {
		side = side_option->arguments()[0].sourceText();
	}
	std::string offset = default_offset;
	if (offset_option != nullptr && !offset_option->arguments().empty()) {
		offset = offset_option->arguments()[0].sourceText();
	}

	std::vector<GeometryExpression> arguments;
	if (axis == "x") {
		arguments.emplace_back("1");
		arguments.emplace_back("0");
		arguments.emplace_back("0");
	}
	else if (axis == "z") {
		arguments.emplace_back("0");
		arguments.emplace_back("0");
		arguments.emplace_back("1");
	}
	else {
		arguments.emplace_back("0");
		arguments.emplace_back("1");
		arguments.emplace_back("0");
	}
	arguments.emplace_back(offset);
	arguments.emplace_back(side);
	return arguments;
}

}

std::shared_ptr<const ShapeDescriptorSyntax>
ShapeAliasExpansionService::expandAlias(
	const ShapeDescriptorSyntax &descriptor,
	std::string *diagnostic) const
{
	const std::string alias_name = descriptor.shapeName();
	if (!ShapeSpecificationParser::isAliasShapeName(alias_name)) {
		if (alias_name == "Cylinder") {
			return std::make_shared<const CylinderShapeDescriptorSyntax>(
				descriptor.options());
		}
		if (alias_name == "Sphere") {
			return std::make_shared<const SphereShapeDescriptorSyntax>(
				descriptor.options());
		}
		if (diagnostic != nullptr) {
			*diagnostic = "Unknown shape family or alias '" + alias_name + "'.";
		}
		return {};
	}

	const std::vector<ShapeOptionSyntax> &source = descriptor.options();
	std::vector<ShapeOptionSyntax> expanded;
	if (alias_name == "Tube") {
		const ShapeOptionSyntax *inner = find_option(source, "inner");
		if (!contains_option(source, "radial") &&
		    !contains_option(source, "wall")) {
			if (inner != nullptr && inner->arguments().size() == 1) {
				expanded.emplace_back("radial",
					std::vector<GeometryExpression>{
						inner->arguments()[0], GeometryExpression("1")});
			}
			else {
				expanded.push_back(option("radial", {"0.75", "1"}));
			}
		}
		if (!contains_option(source, "topology")) {
			expanded.push_back(option("topology", {"shell"}));
		}
		append_passthrough_options(source, {"inner"}, &expanded);
		return std::make_shared<const CylinderShapeDescriptorSyntax>(
			std::move(expanded));
	}

	if (alias_name == "CylinderSector") {
		if (!contains_option(source, "azimuth")) {
			const ShapeOptionSyntax *sweep = find_option(source, "sweep");
			if (sweep != nullptr && sweep->arguments().size() == 1) {
				expanded.emplace_back("azimuth",
					std::vector<GeometryExpression>{
						GeometryExpression("0"), sweep->arguments()[0]});
			}
			else {
				expanded.push_back(option("azimuth", {"0", "180"}));
			}
		}
		append_passthrough_options(source, {"sweep"}, &expanded);
		return std::make_shared<const CylinderShapeDescriptorSyntax>(
			std::move(expanded));
	}

	if (alias_name == "DSection" || alias_name == "HalfCylinder") {
		if (!contains_option(source, "chord")) {
			const ShapeOptionSyntax *angle = find_option(source, "angle");
			const ShapeOptionSyntax *offset = find_option(source, "offset");
			const ShapeOptionSyntax *side = find_option(source, "side");
			std::vector<GeometryExpression> chord_arguments;
			chord_arguments.push_back(
				angle != nullptr ? angle->arguments()[0] : GeometryExpression("0"));
			chord_arguments.push_back(
				offset != nullptr
					? offset->arguments()[0]
					: GeometryExpression(alias_name == "DSection" ? "0.25" : "0"));
			chord_arguments.push_back(
				side != nullptr ? side->arguments()[0] : GeometryExpression("positive"));
			expanded.emplace_back("chord", std::move(chord_arguments));
		}
		append_passthrough_options(source, {"angle", "offset", "side"}, &expanded);
		return std::make_shared<const CylinderShapeDescriptorSyntax>(
			std::move(expanded));
	}

	const ShapeOptionSyntax *axis = find_option(source, "axis");
	const ShapeOptionSyntax *side = find_option(source, "side");
	const ShapeOptionSyntax *offset = find_option(source, "offset");
	const bool has_explicit_clip = contains_option(source, "clip") ||
	                               contains_option(source, "slab");
	if (!has_explicit_clip) {
		const char *default_offset = alias_name == "SphereCap" ? "0.35" : "0";
		expanded.emplace_back(
			"clip", clip_arguments_for_axis(axis, offset, side, default_offset));
	}

	if (alias_name == "SphereBowl") {
		const ShapeOptionSyntax *inner = find_option(source, "inner");
		if (!contains_option(source, "radial") &&
		    !contains_option(source, "wall")) {
			if (inner != nullptr && inner->arguments().size() == 1) {
				expanded.emplace_back("radial",
					std::vector<GeometryExpression>{
						inner->arguments()[0], GeometryExpression("1")});
			}
			else {
				expanded.push_back(option("radial", {"0.85", "1"}));
			}
		}
		if (!contains_option(source, "topology")) {
			expanded.push_back(option("topology", {"shell"}));
		}
		if (!contains_option(source, "close")) {
			expanded.push_back(option("close", {"none"}));
		}
	}

	if (alias_name == "SphereQuarter" && !has_explicit_clip) {
		expanded.clear();
		expanded.push_back(option("clip", {"1", "0", "0", "0", "positive"}));
		expanded.push_back(option("clip", {"0", "1", "0", "0", "positive"}));
	}
	else if (alias_name == "SphereOctant" && !has_explicit_clip) {
		expanded.clear();
		expanded.push_back(option("clip", {"1", "0", "0", "0", "positive"}));
		expanded.push_back(option("clip", {"0", "1", "0", "0", "positive"}));
		expanded.push_back(option("clip", {"0", "0", "1", "0", "positive"}));
	}

	append_passthrough_options(
		source, {"axis", "side", "offset", "inner"}, &expanded);
	return std::make_shared<const SphereShapeDescriptorSyntax>(
		std::move(expanded));
}
