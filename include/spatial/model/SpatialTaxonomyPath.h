#pragma once

#include <string>
#include <utility>
#include <vector>

class SpatialTaxonomyPath {
public:
	SpatialTaxonomyPath() = default;
	explicit SpatialTaxonomyPath(std::vector<std::string> segments)
		: segments_(std::move(segments)) {}

	const std::vector<std::string> &segments() const { return segments_; }
	bool empty() const { return segments_.empty(); }

private:
	std::vector<std::string> segments_;
};
