#include "editor/relationship/SceneSourceAssociationIndex.h"

#include "Context.h"

#include <utility>

SceneSourceAssociationIndex SceneSourceAssociationIndex::fromContext(
	const SceneGenerationContext &context)
{
	std::vector<SceneSourceAssociation> associations;
	associations.reserve(context.primitive_instances.size());
	for (std::size_t index = 0; index < context.primitive_instances.size(); ++index) {
		const ScenePrimitiveInstance &instance = context.primitive_instances[index];
		SceneSourceAssociation association;
		association.primitive_identity.instance_index = static_cast<int>(index);
		association.source_range.start_line = instance.source_start_line;
		association.source_range.start_column = instance.source_start_column;
		association.source_range.end_line = instance.source_end_line;
		association.source_range.end_column = instance.source_end_column;
		if (association.source_range.isValid()) {
			associations.push_back(association);
		}
	}
	return fromAssociations(std::move(associations));
}
