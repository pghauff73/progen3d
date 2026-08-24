#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

class NestedSourceShapeDescriptorSyntax final : public ShapeDescriptorSyntax
{
public:
	NestedSourceShapeDescriptorSyntax(
		std::string shape_name,
		std::shared_ptr<const ShapeDescriptorSyntax> source_descriptor,
		std::vector<ShapeOptionSyntax> options)
		: shape_name_(std::move(shape_name)),
		  source_descriptor_(std::move(source_descriptor)),
		  options_(std::move(options))
	{
	}

	std::string shapeName() const override { return shape_name_; }
	const std::vector<ShapeOptionSyntax> &options() const override { return options_; }
	const std::shared_ptr<const ShapeDescriptorSyntax> &sourceDescriptor() const
	{
		return source_descriptor_;
	}

	std::string canonicalText() const override
	{
		std::ostringstream text;
		text << shape_name_ << "(source(" << source_descriptor_->canonicalText() << ")";
		for (const ShapeOptionSyntax &option : options_) {
			text << " " << option.optionName() << "(";
			for (std::size_t index = 0; index < option.arguments().size(); ++index) {
				if (index > 0) text << " ";
				text << option.arguments()[index].sourceText();
			}
			text << ")";
		}
		text << ")";
		return text.str();
	}

private:
	std::string shape_name_;
	std::shared_ptr<const ShapeDescriptorSyntax> source_descriptor_;
	std::vector<ShapeOptionSyntax> options_;
};
