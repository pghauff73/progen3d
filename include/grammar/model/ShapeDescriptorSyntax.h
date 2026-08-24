#pragma once

#include "grammar/model/ShapeOptionSyntax.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

class ShapeDescriptorSyntax
{
public:
	virtual ~ShapeDescriptorSyntax() = default;

	virtual std::string shapeName() const = 0;
	virtual const std::vector<ShapeOptionSyntax> &options() const = 0;

	virtual std::string canonicalText() const
	{
		std::ostringstream text;
		text << shapeName();
		if (options().empty()) {
			return text.str();
		}

		text << "(";
		for (std::size_t option_index = 0;
		     option_index < options().size();
		     ++option_index) {
			if (option_index > 0) {
				text << " ";
			}
			const ShapeOptionSyntax &option = options()[option_index];
			text << option.optionName() << "(";
			for (std::size_t argument_index = 0;
			     argument_index < option.arguments().size();
			     ++argument_index) {
				if (argument_index > 0) {
					text << " ";
				}
				text << option.arguments()[argument_index].sourceText();
			}
			text << ")";
		}
		text << ")";
		return text.str();
	}
};

class StoredShapeDescriptorSyntax : public ShapeDescriptorSyntax
{
public:
	StoredShapeDescriptorSyntax(std::string shape_name,
	                            std::vector<ShapeOptionSyntax> options)
		: shape_name_(std::move(shape_name)),
		  options_(std::move(options))
	{
	}

	std::string shapeName() const override
	{
		return shape_name_;
	}

	const std::vector<ShapeOptionSyntax> &options() const override
	{
		return options_;
	}

private:
	std::string shape_name_;
	std::vector<ShapeOptionSyntax> options_;
};
