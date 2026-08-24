#include "editor/relationship/SceneSourceAssociationIndex.h"

#include <utility>

SceneSourceAssociationIndex SceneSourceAssociationIndex::fromAssociations(
	std::vector<SceneSourceAssociation> associations)
{
	SceneSourceAssociationIndex index;
	index.associations_ = std::move(associations);
	return index;
}

std::optional<SourceRange> SceneSourceAssociationIndex::sourceRangeForInstance(
	int instance_index) const
{
	for (const SceneSourceAssociation &association : associations_) {
		if (association.primitive_identity.instance_index == instance_index) {
			return association.source_range;
		}
	}
	return std::nullopt;
}

std::vector<ScenePrimitiveIdentity> SceneSourceAssociationIndex::primitivesAtSourcePosition(
	int line,
	int column) const
{
	std::vector<ScenePrimitiveIdentity> identities;
	for (const SceneSourceAssociation &association : associations_) {
		const SourceRange &range = association.source_range;
		if (!range.isValid() || line < range.start_line || line > range.end_line) {
			continue;
		}
		const bool before_start = line == range.start_line && column < range.start_column;
		const bool after_end = line == range.end_line && column > range.end_column;
		if (!before_start && !after_end) {
			identities.push_back(association.primitive_identity);
		}
	}
	return identities;
}

const std::vector<SceneSourceAssociation> &SceneSourceAssociationIndex::associations() const
{
	return associations_;
}
