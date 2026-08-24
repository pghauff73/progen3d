#pragma once

#include "editor/relationship/SceneSourceAssociation.h"

#include <optional>
#include <vector>

class SceneGenerationContext;

class SceneSourceAssociationIndex
{
public:
	static SceneSourceAssociationIndex fromContext(const SceneGenerationContext &context);
	static SceneSourceAssociationIndex fromAssociations(
		std::vector<SceneSourceAssociation> associations);

	std::optional<SourceRange> sourceRangeForInstance(int instance_index) const;
	std::vector<ScenePrimitiveIdentity> primitivesAtSourcePosition(int line,
	                                                              int column) const;
	const std::vector<SceneSourceAssociation> &associations() const;

private:
	std::vector<SceneSourceAssociation> associations_;
};
