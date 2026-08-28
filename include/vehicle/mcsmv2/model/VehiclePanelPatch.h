#pragma once

#include <cstdint>
#include <string>
#include <utility>

enum class VehiclePanelPatchSelectionBasis
{
	SemanticSurfaceTag
};

class VehiclePanelPatch
{
public:
	VehiclePanelPatch(
		std::string identifier,
		std::uint64_t source_face_count,
		bool closure,
		VehiclePanelPatchSelectionBasis selection_basis)
		: identifier_(std::move(identifier)),
		  source_face_count_(source_face_count),
		  closure_(closure),
		  selection_basis_(selection_basis)
	{
	}

	const std::string &identifier() const { return identifier_; }
	std::uint64_t sourceFaceCount() const { return source_face_count_; }
	bool isClosure() const { return closure_; }
	VehiclePanelPatchSelectionBasis selectionBasis() const
	{
		return selection_basis_;
	}

private:
	std::string identifier_;
	std::uint64_t source_face_count_ = 0u;
	bool closure_ = false;
	VehiclePanelPatchSelectionBasis selection_basis_ =
		VehiclePanelPatchSelectionBasis::SemanticSurfaceTag;
};
