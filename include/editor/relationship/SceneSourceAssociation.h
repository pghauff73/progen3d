#pragma once

#include "editor/relationship/ScenePrimitiveIdentity.h"
#include "editor/relationship/SourceRange.h"

class SceneSourceAssociation
{
public:
	ScenePrimitiveIdentity primitive_identity;
	SourceRange source_range;
};
