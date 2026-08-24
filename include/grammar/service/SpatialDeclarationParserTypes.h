#pragma once

#include "grammar/model/GrammarSourceRange.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class SpatialDeclarationParserTypes
{
public:
	using ExpressionArgumentParser =
		std::function<std::string(const std::vector<std::string> &,
		                          std::size_t *,
		                          const std::string &)>;
	using SourceRangeResolver =
		std::function<GrammarSourceRange(std::size_t, std::size_t)>;
};
