#pragma once

#include <cstddef>
#include <string>
#include <utility>

class SurfaceCorrespondenceDirectionReport
{
public:
	SurfaceCorrespondenceDirectionReport(
		std::size_t sample_count,
		double mean_distance,
		double root_mean_square_distance,
		double percentile95_distance,
		double maximum_distance,
		double mean_normal_angle_degrees,
		double percentile95_normal_angle_degrees,
		double maximum_normal_angle_degrees,
		double minimum_normal_dot,
		std::string closest_point_checksum)
		: sample_count_(sample_count),
		  mean_distance_(mean_distance),
		  root_mean_square_distance_(root_mean_square_distance),
		  percentile95_distance_(percentile95_distance),
		  maximum_distance_(maximum_distance),
		  mean_normal_angle_degrees_(mean_normal_angle_degrees),
		  percentile95_normal_angle_degrees_(percentile95_normal_angle_degrees),
		  maximum_normal_angle_degrees_(maximum_normal_angle_degrees),
		  minimum_normal_dot_(minimum_normal_dot),
		  closest_point_checksum_(std::move(closest_point_checksum))
	{
	}

	std::size_t sampleCount() const { return sample_count_; }
	double meanDistance() const { return mean_distance_; }
	double rootMeanSquareDistance() const { return root_mean_square_distance_; }
	double percentile95Distance() const { return percentile95_distance_; }
	double maximumDistance() const { return maximum_distance_; }
	double meanNormalAngleDegrees() const { return mean_normal_angle_degrees_; }
	double percentile95NormalAngleDegrees() const
	{
		return percentile95_normal_angle_degrees_;
	}
	double maximumNormalAngleDegrees() const { return maximum_normal_angle_degrees_; }
	double minimumNormalDot() const { return minimum_normal_dot_; }
	const std::string &closestPointChecksum() const { return closest_point_checksum_; }

private:
	std::size_t sample_count_ = 0u;
	double mean_distance_ = 0.0;
	double root_mean_square_distance_ = 0.0;
	double percentile95_distance_ = 0.0;
	double maximum_distance_ = 0.0;
	double mean_normal_angle_degrees_ = 0.0;
	double percentile95_normal_angle_degrees_ = 0.0;
	double maximum_normal_angle_degrees_ = 0.0;
	double minimum_normal_dot_ = 1.0;
	std::string closest_point_checksum_;
};
