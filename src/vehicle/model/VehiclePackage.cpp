#include "vehicle/model/VehiclePackage.h"

#include <cmath>

bool VehiclePackageEnvelope::isFiniteAndOrdered() const
{
	const bool finite =
		std::isfinite(minimum_.x) && std::isfinite(minimum_.y) &&
		std::isfinite(minimum_.z) && std::isfinite(maximum_.x) &&
		std::isfinite(maximum_.y) && std::isfinite(maximum_.z);
	return finite && minimum_.x <= maximum_.x && minimum_.y <= maximum_.y &&
	       minimum_.z <= maximum_.z;
}
