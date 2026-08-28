#pragma once

enum class SpatialObjectState {
	Unplaced,
	Approximate,
	Positioned,
	ContactResolved,
	Connected,
	Constrained,
	Validated,
	Invalid
};
