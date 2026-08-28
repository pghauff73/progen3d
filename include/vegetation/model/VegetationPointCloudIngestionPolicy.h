#pragma once

#include <cstddef>

class VegetationPointCloudIngestionPolicy
{
public:
	VegetationPointCloudIngestionPolicy(
		std::size_t maximum_payload_bytes,
		std::size_t maximum_point_count,
		bool reject_duplicate_points)
		: maximum_payload_bytes_(maximum_payload_bytes),
		  maximum_point_count_(maximum_point_count),
		  reject_duplicate_points_(reject_duplicate_points)
	{
	}

	std::size_t maximumPayloadBytes() const { return maximum_payload_bytes_; }
	std::size_t maximumPointCount() const { return maximum_point_count_; }
	bool rejectDuplicatePoints() const { return reject_duplicate_points_; }

private:
	std::size_t maximum_payload_bytes_ = 0u;
	std::size_t maximum_point_count_ = 0u;
	bool reject_duplicate_points_ = false;
};
