#pragma once

#include "geometry/model/ShapeSpecification.h"
#include "grammar/model/GeometryExpression.h"
#include "grammar/model/ShapeDescriptorSyntax.h"

#include <functional>
#include <memory>
#include <string>

struct ShapeSpecificationEvaluationResult
{
	std::shared_ptr<const ShapeSpecification> specification;
	std::string diagnostic;

	bool succeeded() const
	{
		return specification != nullptr;
	}
};

class ShapeSpecificationEvaluator
{
public:
	using ExpressionEvaluator =
		std::function<bool(const GeometryExpression &,
		                   const std::string &,
		                   float *)>;

	ShapeSpecificationEvaluationResult evaluate(
		const ShapeDescriptorSyntax &descriptor,
		const ExpressionEvaluator &expression_evaluator) const;
};
