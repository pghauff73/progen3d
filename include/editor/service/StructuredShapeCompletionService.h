#pragma once

#include "editor/model/ShapeCompletionContext.h"

#include <string>

class StructuredShapeCompletionService
{
public:
	ShapeCompletionContext analyze(const std::string &source_before_cursor) const;
};
