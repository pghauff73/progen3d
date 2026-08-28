#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/ShapeDescriptorSyntax.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

class Profile2DConstructorSyntax
{
public:
	Profile2DConstructorSyntax(
		std::string constructor_name,
		std::vector<GeometryExpression> arguments)
		: constructor_name_(std::move(constructor_name)),
		  arguments_(std::move(arguments))
	{
	}

	const std::string &constructorName() const { return constructor_name_; }
	const std::vector<GeometryExpression> &arguments() const { return arguments_; }

private:
	std::string constructor_name_;
	std::vector<GeometryExpression> arguments_;
};

class ExtrudeProfileDescriptorSyntax : public ShapeDescriptorSyntax
{
public:
	ExtrudeProfileDescriptorSyntax(
		Profile2DConstructorSyntax profile,
		GeometryExpression depth,
		std::string cap_name)
		: profile_(std::move(profile)),
		  depth_(std::move(depth)),
		  cap_name_(std::move(cap_name))
	{
	}

	std::string shapeName() const override { return "Extrude"; }
	const std::vector<ShapeOptionSyntax> &options() const override
	{
		static const std::vector<ShapeOptionSyntax> no_options;
		return no_options;
	}
	const Profile2DConstructorSyntax &profile() const { return profile_; }
	const GeometryExpression &depth() const { return depth_; }
	const std::string &capName() const { return cap_name_; }

	std::string canonicalText() const override
	{
		std::ostringstream text;
		text << "Extrude(" << profile_.constructorName() << "(";
		for (std::size_t index = 0; index < profile_.arguments().size(); ++index) {
			if (index > 0) text << " ";
			text << profile_.arguments()[index].sourceText();
		}
		text << ") " << depth_.sourceText() << " cap(" << cap_name_ << "))";
		return text.str();
	}

private:
	Profile2DConstructorSyntax profile_{{}, {}};
	GeometryExpression depth_{"0"};
	std::string cap_name_ = "all";
};
