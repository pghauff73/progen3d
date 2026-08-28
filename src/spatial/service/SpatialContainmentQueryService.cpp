#include "spatial/service/SpatialContainmentQueryService.h"

bool SpatialContainmentQueryService::contains(
	const AxisAlignedBounds &container,
	const AxisAlignedBounds &contained,
	float tolerance) const
{
	return container.valid && contained.valid &&
	       contained.min.x >= container.min.x - tolerance &&
	       contained.min.y >= container.min.y - tolerance &&
	       contained.min.z >= container.min.z - tolerance &&
	       contained.max.x <= container.max.x + tolerance &&
	       contained.max.y <= container.max.y + tolerance &&
	       contained.max.z <= container.max.z + tolerance;
}

bool SpatialContainmentQueryService::isAbove(
	const AxisAlignedBounds &first,
	const AxisAlignedBounds &second,
	float tolerance) const
{
	return first.valid && second.valid && first.min.y >= second.max.y - tolerance;
}

bool SpatialContainmentQueryService::isBelow(
	const AxisAlignedBounds &first,
	const AxisAlignedBounds &second,
	float tolerance) const
{
	return first.valid && second.valid && first.max.y <= second.min.y + tolerance;
}
