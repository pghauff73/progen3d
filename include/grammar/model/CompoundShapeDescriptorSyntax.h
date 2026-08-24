#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/ShapeDescriptorSyntax.h"

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

class CompoundShapePartSyntax
{
public:
	CompoundShapePartSyntax(
		std::string purpose,
		std::shared_ptr<const ShapeDescriptorSyntax> source_descriptor,
		std::vector<GeometryExpression> transform_arguments)
		: purpose_(std::move(purpose)),
		  source_descriptor_(std::move(source_descriptor)),
		  transform_arguments_(std::move(transform_arguments))
	{
	}

	const std::string &purpose() const { return purpose_; }
	const std::shared_ptr<const ShapeDescriptorSyntax> &sourceDescriptor() const
	{
		return source_descriptor_;
	}
	const std::vector<GeometryExpression> &transformArguments() const
	{
		return transform_arguments_;
	}

private:
	std::string purpose_;
	std::shared_ptr<const ShapeDescriptorSyntax> source_descriptor_;
	std::vector<GeometryExpression> transform_arguments_;
};

class CompoundShapeDescriptorSyntax final : public ShapeDescriptorSyntax
{
public:
	CompoundShapeDescriptorSyntax(
		std::vector<CompoundShapePartSyntax> parts,
		GeometryExpression detail)
		: parts_(std::move(parts)), detail_(std::move(detail))
	{
	}

	std::string shapeName() const override { return "CompoundShape"; }
	const std::vector<ShapeOptionSyntax> &options() const override
	{
		static const std::vector<ShapeOptionSyntax> no_options;
		return no_options;
	}
	const std::vector<CompoundShapePartSyntax> &parts() const { return parts_; }
	const GeometryExpression &detail() const { return detail_; }

	std::string canonicalText() const override
	{
		std::ostringstream text;
		text << "CompoundShape(";
		for (const CompoundShapePartSyntax &part : parts_) {
			text << "part(" << part.purpose() << " source("
			     << part.sourceDescriptor()->canonicalText() << ") transform(";
			for (std::size_t index = 0;
			     index < part.transformArguments().size();
			     ++index) {
				if (index > 0) text << " ";
				text << part.transformArguments()[index].sourceText();
			}
			text << ")) ";
		}
		text << "detail(" << detail_.sourceText() << "))";
		return text.str();
	}

private:
	std::vector<CompoundShapePartSyntax> parts_;
	GeometryExpression detail_{"LOD3"};
};
