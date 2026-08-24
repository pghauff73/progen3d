#pragma once

#include "grammar/model/LightingDeclarationSyntax.h"
#include "grammar/service/SpatialDeclarationParserTypes.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class LightingDeclarationParser
{
public:
	std::shared_ptr<const SceneLightDeclarationSyntax> parseLight(
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		std::size_t declaration_start_index,
		const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
		const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
		std::string *diagnostic) const;

	std::shared_ptr<const LightFixtureDeclarationSyntax> parseFixture(
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		std::size_t declaration_start_index,
		const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
		const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
		std::string *diagnostic) const;

	std::shared_ptr<const LightSwitchDeclarationSyntax> parseSwitch(
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		std::size_t declaration_start_index,
		const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
		const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
		std::string *diagnostic) const;

	std::shared_ptr<const LightingArrayDeclarationSyntax> parseArray(
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		std::size_t declaration_start_index,
		const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
		const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
		std::string *diagnostic) const;

	std::shared_ptr<const LightingCircuitDeclarationSyntax> parseCircuit(
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		std::size_t declaration_start_index,
		const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
		std::string *diagnostic) const;

	std::shared_ptr<const ControlsDeclarationSyntax> parseControls(
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		std::size_t declaration_start_index,
		const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
		std::string *diagnostic) const;
};
