#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/ShapeDescriptorSyntax.h"

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

enum class InstanceArrayPatternKind
{
	Linear,
	Grid,
	Radial
};

class InstanceArrayPatternSyntax
{
public:
	InstanceArrayPatternSyntax(
		InstanceArrayPatternKind kind,
		std::vector<GeometryExpression> arguments)
		: kind_(kind), arguments_(std::move(arguments))
	{
	}

	InstanceArrayPatternKind kind() const { return kind_; }
	const std::vector<GeometryExpression> &arguments() const { return arguments_; }

	std::string patternName() const
	{
		if (kind_ == InstanceArrayPatternKind::Linear) return "linear";
		if (kind_ == InstanceArrayPatternKind::Grid) return "grid";
		return "radial";
	}

private:
	InstanceArrayPatternKind kind_ = InstanceArrayPatternKind::Linear;
	std::vector<GeometryExpression> arguments_;
};

class InstanceArrayDescriptorSyntax final : public ShapeDescriptorSyntax
{
public:
	InstanceArrayDescriptorSyntax(
		std::shared_ptr<const ShapeDescriptorSyntax> source_descriptor,
		InstanceArrayPatternSyntax pattern,
		GeometryExpression detail)
		: source_descriptor_(std::move(source_descriptor)),
		  pattern_(std::move(pattern)),
		  detail_(std::move(detail))
	{
	}

	std::string shapeName() const override { return "InstanceArray"; }
	const std::vector<ShapeOptionSyntax> &options() const override
	{
		static const std::vector<ShapeOptionSyntax> no_options;
		return no_options;
	}

	const std::shared_ptr<const ShapeDescriptorSyntax> &sourceDescriptor() const
	{
		return source_descriptor_;
	}
	const InstanceArrayPatternSyntax &pattern() const { return pattern_; }
	const GeometryExpression &detail() const { return detail_; }

	std::string canonicalText() const override
	{
		std::ostringstream text;
		text << "InstanceArray(source(" << source_descriptor_->canonicalText() << ") "
		     << pattern_.patternName() << "(";
		for (std::size_t index = 0; index < pattern_.arguments().size(); ++index) {
			if (index > 0) text << " ";
			text << pattern_.arguments()[index].sourceText();
		}
		text << ") detail(" << detail_.sourceText() << "))";
		return text.str();
	}

private:
	std::shared_ptr<const ShapeDescriptorSyntax> source_descriptor_;
	InstanceArrayPatternSyntax pattern_{InstanceArrayPatternKind::Linear, {}};
	GeometryExpression detail_{"LOD5"};
};
