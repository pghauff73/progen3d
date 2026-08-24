#pragma once

#include "building/relationship/BuildingRelationshipAssertionReference.h"

#include <utility>
#include <vector>

class BuildingRelationshipAssertionModel {
public:
	BuildingRelationshipAssertionModel() = default;
	explicit BuildingRelationshipAssertionModel(
		std::vector<BuildingRelationshipAssertionReference> assertions)
		: assertions_(std::move(assertions)) {}

	const std::vector<BuildingRelationshipAssertionReference> &assertions() const
	{
		return assertions_;
	}

	const BuildingRelationshipAssertionReference *findAssertion(
		const SpatialObjectId &assertion_object_id) const
	{
		for (const BuildingRelationshipAssertionReference &assertion : assertions_) {
			if (assertion.assertionObjectId() == assertion_object_id) return &assertion;
		}
		return nullptr;
	}

private:
	std::vector<BuildingRelationshipAssertionReference> assertions_;
};
