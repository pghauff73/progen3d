#pragma once

#include "geometry/service/GeneratedMeshProvider.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class DocumentMeshLibrary final : public GeneratedMeshProvider
{
public:
	// Leaves source_line on the offending row if a block is malformed.
	static std::shared_ptr<const DocumentMeshLibrary> read(
		const std::vector<std::string> &lines, int &source_line);

	bool empty() const { return meshes_.empty(); }
	bool recognizes(const std::string &key) const override;
	GeneratedMeshResolutionResult resolve(
		const std::string &key, GeometryDetailLevel detail_level) const override;

private:
	std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes_;
};
