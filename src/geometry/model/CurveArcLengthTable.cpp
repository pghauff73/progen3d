#include "geometry/model/CurveArcLengthTable.h"

#include <algorithm>

double CurveArcLengthTable::parameterAtDistance(double distance) const
{
	if (samples_.empty()) return 0.0;
	distance = std::clamp(distance, 0.0, totalLength());
	const auto upper = std::lower_bound(
		samples_.begin(), samples_.end(), distance,
		[](const CurveArcLengthSample &sample, double requested_distance) {
			return sample.distance() < requested_distance;
		});
	if (upper == samples_.begin()) return upper->parameter();
	if (upper == samples_.end()) return samples_.back().parameter();
	const CurveArcLengthSample &second = *upper;
	const CurveArcLengthSample &first = *(upper - 1);
	const double interval = second.distance() - first.distance();
	if (interval <= 0.0) return first.parameter();
	const double fraction = (distance - first.distance()) / interval;
	return first.parameter() + (second.parameter() - first.parameter()) * fraction;
}
