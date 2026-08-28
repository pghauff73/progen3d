#include "vehicle/model/VehicleDatum.h"

const VehicleDatum *VehicleDatumSet::find(VehicleDatumType type) const
{
	for (const VehicleDatum &datum : datums_) {
		if (datum.type() == type) return &datum;
	}
	return nullptr;
}
