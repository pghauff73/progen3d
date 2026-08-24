#include "spatial/service/SpatialWorldFrameResolutionService.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

std::optional<SpatialObjectRegistry> SpatialWorldFrameResolutionService::resolve(
	const SpatialObjectRegistry &objects,
	const SpatialContainmentTree &containment_tree,
	SpatialModelValidationReport *validation_report) const
{
	const SpatialBuildingObject *root = objects.find(containment_tree.rootObjectId());
	if (root == nullptr) {
		if (validation_report != nullptr) {
			validation_report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::MissingRootObject,
				"Cannot resolve spatial world frames because the root object is missing."));
		}
		return std::nullopt;
	}

	std::map<SpatialObjectId, glm::mat4> world_transforms;
	std::vector<SpatialObjectId> pending{containment_tree.rootObjectId()};
	std::set<SpatialObjectId> visited;
	while (!pending.empty()) {
		const SpatialObjectId object_id = pending.back();
		pending.pop_back();
		if (!visited.insert(object_id).second) {
			if (validation_report != nullptr) {
				validation_report->addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::ContainmentCycle,
					"Cannot resolve spatial world frames because containment revisits object '" +
						object_id.value() + "'.",
					{object_id}));
			}
			return std::nullopt;
		}

		const SpatialBuildingObject *object = objects.find(object_id);
		if (object == nullptr) {
			if (validation_report != nullptr) {
				validation_report->addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::MissingContainer,
					"Cannot resolve spatial world frame for undefined object '" +
						object_id.value() + "'.",
					{object_id}));
			}
			return std::nullopt;
		}

		glm::mat4 parent_world(1.0f);
		const SpatialObjectId *parent_id = containment_tree.parentOf(object_id);
		if (parent_id != nullptr) {
			const auto parent_world_found = world_transforms.find(*parent_id);
			if (parent_world_found == world_transforms.end()) {
				if (validation_report != nullptr) {
					validation_report->addIssue(SpatialModelValidationIssue(
						SpatialModelValidationCode::FrameParentMismatch,
						"Parent world frame for object '" + object_id.value() +
							"' was not resolved first.",
						{*parent_id, object_id}));
				}
				return std::nullopt;
			}
			parent_world = parent_world_found->second;
		}

		world_transforms[object_id] =
			parent_world * object->frameState().authoredLocalTransform() *
			object->frameState().resolutionLocalTransform();

		std::vector<SpatialObjectId> children = containment_tree.childrenOf(object_id);
		std::sort(children.begin(), children.end());
		for (auto child = children.rbegin(); child != children.rend(); ++child) {
			pending.push_back(*child);
		}
	}

	if (visited.size() != objects.size()) {
		if (validation_report != nullptr) {
			validation_report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::MissingContainer,
				"Spatial world-frame resolution did not reach every object."));
		}
		return std::nullopt;
	}

	std::vector<SpatialBuildingObject> resolved_objects;
	resolved_objects.reserve(objects.size());
	for (const SpatialBuildingObject &object : objects.objects()) {
		const glm::mat4 &world = world_transforms.at(object.identity().objectId());
		resolved_objects.push_back(object.withFrameState(
			object.frameState().withResolvedPlacement(
				object.frameState().resolutionLocalTransform(), world),
			object.state()));
	}
	return SpatialObjectRegistry(std::move(resolved_objects));
}
