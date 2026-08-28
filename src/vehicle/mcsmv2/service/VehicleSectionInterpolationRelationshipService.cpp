#include "vehicle/mcsmv2/service/VehicleSectionInterpolationRelationshipService.h"

#include <algorithm>
#include <stdexcept>

VehicleSectionInterpolationRelationship
VehicleSectionInterpolationRelationshipService::resolve(
	const VehicleSemanticSectionField &section_field,
	double source_x) const
{
	const std::vector<VehicleSemanticSectionStation> &stations =
		section_field.stations();
	if (stations.size() < 2u) {
		throw std::invalid_argument(
			"Section interpolation provenance requires at least two stations.");
	}
	std::size_t first_index = 0u;
	if (source_x <= stations.front().sourceX()) {
		first_index = 0u;
	}
	else if (source_x >= stations.back().sourceX()) {
		first_index = stations.size() - 2u;
	}
	else {
		const auto upper = std::upper_bound(
			stations.begin(), stations.end(), source_x,
			[](double value, const VehicleSemanticSectionStation &station) {
				return value < station.sourceX();
			});
		first_index = static_cast<std::size_t>(
			std::distance(stations.begin(), upper) - 1);
	}
	const std::size_t second_index = first_index + 1u;
	const double first_x = stations[first_index].sourceX();
	const double second_x = stations[second_index].sourceX();
	const double parameter = (source_x - first_x) / (second_x - first_x);
	return VehicleSectionInterpolationRelationship(
		source_x, first_index, second_index, parameter);
}
