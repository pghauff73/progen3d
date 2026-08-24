#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

#include <string>
#include <utility>

class BotanicalBladeShapeDescriptorSyntax : public StoredShapeDescriptorSyntax
{
protected:
	BotanicalBladeShapeDescriptorSyntax(
		std::string shape_name,
		std::vector<ShapeOptionSyntax> options)
		: StoredShapeDescriptorSyntax(
			  std::move(shape_name),
			  std::move(options))
	{
	}
};

class LeafBladeShapeDescriptorSyntax final
	: public BotanicalBladeShapeDescriptorSyntax
{
public:
	explicit LeafBladeShapeDescriptorSyntax(
		std::vector<ShapeOptionSyntax> options)
		: BotanicalBladeShapeDescriptorSyntax(
			  "LeafBlade",
			  std::move(options))
	{
	}
};

class PetalBladeShapeDescriptorSyntax final
	: public BotanicalBladeShapeDescriptorSyntax
{
public:
	explicit PetalBladeShapeDescriptorSyntax(
		std::vector<ShapeOptionSyntax> options)
		: BotanicalBladeShapeDescriptorSyntax(
			  "PetalBlade",
			  std::move(options))
	{
	}
};
