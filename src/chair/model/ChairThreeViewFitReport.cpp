#include "chair/model/ChairThreeViewFitReport.h"

#include <algorithm>

bool ChairThreeViewFitReport::passes(double threshold) const
{
	return views_.size() == 3u && std::all_of(
		views_.begin(), views_.end(),
		[threshold](const ChairViewFitRecord &record) {
			return record.passes(threshold);
		});
}
