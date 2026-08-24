#pragma once

#include "editor/model/SpatialDeclarationCompletionContext.h"

#include <string>

class SpatialDeclarationCompletionService
{
public:
	SpatialDeclarationCompletionContext analyze(
		const std::string &source_before_cursor) const;
};
