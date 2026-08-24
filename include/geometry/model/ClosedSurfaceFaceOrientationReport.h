#pragma once

#include <cstddef>
#include <string>
#include <utility>

class ClosedSurfaceFaceOrientationReport
{
public:
	static ClosedSurfaceFaceOrientationReport createSuccess(
		std::size_t connected_component_count,
		std::size_t flipped_face_count)
	{
		return ClosedSurfaceFaceOrientationReport(
			true,
			connected_component_count,
			flipped_face_count,
			{});
	}

	static ClosedSurfaceFaceOrientationReport createFailure(std::string diagnostic)
	{
		return ClosedSurfaceFaceOrientationReport(
			false,
			0u,
			0u,
			std::move(diagnostic));
	}

	bool succeeded() const { return succeeded_; }
	std::size_t connectedComponentCount() const
	{
		return connected_component_count_;
	}
	std::size_t flippedFaceCount() const { return flipped_face_count_; }
	const std::string &diagnostic() const { return diagnostic_; }

private:
	ClosedSurfaceFaceOrientationReport(
		bool succeeded,
		std::size_t connected_component_count,
		std::size_t flipped_face_count,
		std::string diagnostic)
		: succeeded_(succeeded),
		  connected_component_count_(connected_component_count),
		  flipped_face_count_(flipped_face_count),
		  diagnostic_(std::move(diagnostic))
	{
	}

	bool succeeded_ = false;
	std::size_t connected_component_count_ = 0u;
	std::size_t flipped_face_count_ = 0u;
	std::string diagnostic_;
};
