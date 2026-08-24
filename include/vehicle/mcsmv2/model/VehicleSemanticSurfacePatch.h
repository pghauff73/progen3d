#pragma once

#include <cstddef>

class VehicleSemanticSurfacePatch
{
public:
	VehicleSemanticSurfacePatch(
		std::size_t first_section_index,
		std::size_t second_section_index,
		std::size_t first_face_index,
		std::size_t face_count)
		: first_section_index_(first_section_index),
		  second_section_index_(second_section_index),
		  first_face_index_(first_face_index),
		  face_count_(face_count)
	{
	}

	std::size_t firstSectionIndex() const { return first_section_index_; }
	std::size_t secondSectionIndex() const { return second_section_index_; }
	std::size_t firstFaceIndex() const { return first_face_index_; }
	std::size_t faceCount() const { return face_count_; }

private:
	std::size_t first_section_index_ = 0u;
	std::size_t second_section_index_ = 0u;
	std::size_t first_face_index_ = 0u;
	std::size_t face_count_ = 0u;
};
