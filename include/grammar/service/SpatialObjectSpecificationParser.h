#pragma once

#include "grammar/model/SpatialObjectDescriptorSyntax.h"
#include "grammar/service/SpatialDeclarationParserTypes.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class SpatialObjectSpecificationParser
{
public:
	std::shared_ptr<const SpatialObjectDescriptorSyntax> parse(
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		std::size_t declaration_start_index,
		const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
		std::string *diagnostic) const;
};
