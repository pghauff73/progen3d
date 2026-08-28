#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class ShapeSpecificationParser
{
public:
	using ExpressionArgumentParser =
		std::function<std::string(const std::vector<std::string> &,
		                          std::size_t *,
		                          const std::string &)>;

	static bool isSupportedShapeName(const std::string &shape_name);
	static bool isCanonicalShapeName(const std::string &shape_name);
	static bool isAliasShapeName(const std::string &shape_name);

	std::shared_ptr<const ShapeDescriptorSyntax> createBareDescriptor(
		const std::string &shape_name,
		std::string *diagnostic) const;

	std::shared_ptr<const ShapeDescriptorSyntax> parseNestedDescriptor(
		const std::string &shape_name,
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		const ExpressionArgumentParser &expression_parser,
		std::string *diagnostic) const;

private:
	bool validateOption(const std::string &shape_name,
	                    const ShapeOptionSyntax &option,
	                    std::string *diagnostic) const;
};
