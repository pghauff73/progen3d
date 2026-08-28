#include "vegetation/service/VegetationPointCloudReconstructionAdmissionService.h"

#include <cmath>
#include <set>
#include <string>
#include <tuple>
#include <vector>

VegetationPointCloudReconstructionAdmissionReport
VegetationPointCloudReconstructionAdmissionService::evaluate(
	const VegetationPointCloudDataset &dataset,
	const VegetationPointCloudReconstructionAdmissionPolicy &policy) const
{
	std::set<std::tuple<double, double, double>> unique_positions;
	std::size_t duplicate_count = 0u;
	std::size_t woody_count = 0u;
	std::size_t foliage_count = 0u;
	std::size_t unknown_count = 0u;
	for (const auto &point : dataset.points()) {
		const auto &position = point.positionMetres();
		if (!unique_positions
		         .insert(std::make_tuple(
			         position.x(), position.y(), position.z()))
		         .second) {
			++duplicate_count;
		}
		switch (point.organClass()) {
		case VegetationPointCloudOrganClass::Woody: ++woody_count; break;
		case VegetationPointCloudOrganClass::Foliage: ++foliage_count; break;
		case VegetationPointCloudOrganClass::Unknown: ++unknown_count; break;
		}
	}
	const double duplicate_fraction = dataset.points().empty()
		                                      ? 0.0
		                                      : static_cast<double>(duplicate_count) /
			                                        dataset.points().size();
	std::vector<std::string> rejection_reasons;
	if (policy.minimumPointCount() == 0u ||
	    !std::isfinite(policy.minimumAxisExtentMetres()) ||
	    policy.minimumAxisExtentMetres() < 0.0 ||
	    !std::isfinite(policy.maximumDuplicateFraction()) ||
	    policy.maximumDuplicateFraction() < 0.0 ||
	    policy.maximumDuplicateFraction() > 1.0) {
		rejection_reasons.emplace_back(
			"Reconstruction admission policy is invalid.");
	}
	if (dataset.datasetIdentifier().empty()) {
		rejection_reasons.emplace_back("Dataset identifier is empty.");
	}
	if (dataset.sourceMetadata().sourcePayloadSha256().empty()) {
		rejection_reasons.emplace_back("Dataset source payload hash is empty.");
	}
	if (dataset.points().size() < policy.minimumPointCount()) {
		rejection_reasons.emplace_back(
			"Dataset point count is below the reconstruction minimum.");
	}
	const auto &bounds = dataset.observedBounds();
	if (!bounds.valid() || bounds.extentX() < policy.minimumAxisExtentMetres() ||
	    bounds.extentY() < policy.minimumAxisExtentMetres() ||
	    bounds.extentZ() < policy.minimumAxisExtentMetres()) {
		rejection_reasons.emplace_back(
			"Dataset does not occupy the required three-dimensional extent.");
	}
	if (duplicate_fraction > policy.maximumDuplicateFraction()) {
		rejection_reasons.emplace_back(
			"Dataset duplicate fraction exceeds the reconstruction boundary.");
	}
	if (policy.requireWoodyAndFoliageLabels() &&
	    (woody_count == 0u || foliage_count == 0u)) {
		rejection_reasons.emplace_back(
			"Dataset lacks the required woody and foliage organ labels.");
	}
	return VegetationPointCloudReconstructionAdmissionReport(
		dataset.datasetIdentifier(),
		dataset.sourceMetadata().sourcePayloadSha256(), policy.target(),
		dataset.points().size(), duplicate_count, woody_count, foliage_count,
		unknown_count, duplicate_fraction, bounds, std::move(rejection_reasons));
}
