#include "vegetation/service/VegetationPointCloudIngestionService.h"

#include "vegetation/service/VegetationLasPointCloudDecoder.h"
#include "vegetation/service/VegetationPlyPointCloudDecoder.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

VegetationPointCloudIngestionReport unsupported_report(
	VegetationPointCloudIngestionIssueCode code,
	const std::string &message)
{
	std::vector<VegetationPointCloudIngestionIssue> issues;
	issues.emplace_back(code, std::string(), message);
	return VegetationPointCloudIngestionReport(
		"VegetationPointCloudIngestionService", std::move(issues), {},
		std::nullopt, std::nullopt);
}

}

VegetationPointCloudIngestionReport VegetationPointCloudIngestionService::ingest(
	const VegetationMeasuredSourceArtifact &artifact,
	const VegetationPointCloudIngestionContext &context) const
{
	if (artifact.mediaType() == "application/ply") {
		return VegetationPlyPointCloudDecoder().decode(artifact, context);
	}
	if (artifact.mediaType() == "application/vnd.las") {
		return VegetationLasPointCloudDecoder().decode(artifact, context);
	}
	if (artifact.mediaType() == "application/vnd.laz") {
		return unsupported_report(
			VegetationPointCloudIngestionIssueCode::MissingRequiredDependency,
			"LAZ ingestion requires an audited LASzip or PDAL integration.");
	}
	if (artifact.mediaType() == "model/e57") {
		return unsupported_report(
			VegetationPointCloudIngestionIssueCode::MissingRequiredDependency,
			"E57 ingestion requires an audited libE57Format integration.");
	}
	return unsupported_report(
		VegetationPointCloudIngestionIssueCode::UnsupportedMediaType,
		"Point-cloud media type has no exact D3A decoder registration.");
}
