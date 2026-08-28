#include "vegetation/service/VegetationAutomatedWoodySegmentationEnsembleService.h"

#include "vegetation/service/VegetationSegmentedWoodyBranchGraphReconstructionService.h"
#include "vegetation/service/VegetationWoodyCoverSetSegmentationService.h"
#include "vegetation/service/VegetationWoodySegmentationSensitivityEvaluationService.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char *job_schema =
	"ProGen3D-VegetationPointCloudReconstructionJob-v1";
constexpr const char *ensemble_algorithm =
	"ProGen3D-AutomatedWoodySegmentationEnsemble-v1";
constexpr const char *graph_algorithm =
	"ProGen3D-SegmentedWoodyBranchGraph-v1";
constexpr double pi = 3.141592653589793238462643383279502884;

using IssueCode = VegetationAutomatedWoodySegmentationEnsembleIssueCode;

std::uint64_t stable_identifier_hash(const std::string &identifier)
{
	std::uint64_t hash = 1469598103934665603ull;
	for (const unsigned char character : identifier) {
		hash ^= character;
		hash *= 1099511628211ull;
	}
	return hash;
}

double distance(
	const VegetationMeasuredPoint3d &first,
	const VegetationMeasuredPoint3d &second)
{
	const double x = first.x() - second.x();
	const double y = first.y() - second.y();
	const double z = first.z() - second.z();
	return std::sqrt(x * x + y * y + z * z);
}

std::pair<double, double> graph_dimensions(
	const VegetationWoodyBranchGraphReconstruction &reconstruction)
{
	double total_length = 0.0;
	double total_volume = 0.0;
	for (const auto &axis : reconstruction.axes()) {
		for (const auto &cylinder : axis.cylinders()) {
			const double cylinder_length = distance(
				cylinder.startPointMetres(), cylinder.endPointMetres());
			total_length += cylinder_length;
			total_volume += pi * cylinder.radiusMetres() *
			                cylinder.radiusMetres() * cylinder_length;
		}
	}
	return std::make_pair(total_length, total_volume);
}

VegetationAutomatedWoodySegmentationEnsembleReport report(
	std::vector<VegetationAutomatedWoodySegmentationEnsembleIssue> issues,
	std::vector<VegetationWoodySegmentationCandidateEvaluation> evaluations = {},
	std::optional<VegetationWoodySegmentationSensitivityReport>
		sensitivity_report = std::nullopt,
	std::optional<VegetationWoodySegmentationCandidate> selected_candidate =
		std::nullopt,
	std::optional<VegetationWoodyBranchGraphReconstruction>
		selected_reconstruction = std::nullopt)
{
	return VegetationAutomatedWoodySegmentationEnsembleReport(
		std::move(issues), std::move(evaluations),
		std::move(sensitivity_report), std::move(selected_candidate),
		std::move(selected_reconstruction));
}

}

VegetationAutomatedWoodySegmentationEnsembleReport
VegetationAutomatedWoodySegmentationEnsembleService::reconstruct(
	const std::string &ensemble_identifier,
	const VegetationPointCloudDataset &dataset,
	const VegetationPointCloudReconstructionAdmissionReport &admission_report,
	const VegetationPointCloudReconstructionJob &job,
	const std::vector<VegetationWoodySegmentationParameterSet> &parameter_sets,
	const VegetationWoodyBranchGraphReconstructionPolicy &graph_policy,
	const VegetationWoodySegmentationSensitivityPolicy &sensitivity_policy) const
{
	std::vector<VegetationAutomatedWoodySegmentationEnsembleIssue> issues;
	if (ensemble_identifier.empty()) {
		issues.emplace_back(
			IssueCode::InvalidEnsembleIdentifier,
			"Automated woody segmentation ensemble identity is empty.");
	}
	if (job.schemaVersion() != job_schema) {
		issues.emplace_back(
			IssueCode::UnsupportedJobSchema,
			"Automated woody segmentation ensemble job schema is unsupported.");
	}
	if (job.target() != VegetationPointCloudReconstructionTarget::ShootArchitecture) {
		issues.emplace_back(
			IssueCode::WrongReconstructionTarget,
			"Automated woody segmentation ensemble requires ShootArchitecture intent.");
	}
	if (job.algorithmIdentifier() != ensemble_algorithm) {
		issues.emplace_back(
			IssueCode::UnsupportedAlgorithm,
			"Automated woody segmentation ensemble algorithm is unsupported.");
	}
	if (job.datasetIdentifier() != dataset.datasetIdentifier() ||
	    admission_report.datasetIdentifier() != dataset.datasetIdentifier()) {
		issues.emplace_back(
			IssueCode::DatasetIdentityMismatch,
			"Dataset, admission, and ensemble job identities differ.");
	}
	if (job.sourcePayloadSha256() !=
		    dataset.sourceMetadata().sourcePayloadSha256() ||
	    admission_report.sourcePayloadSha256() !=
		    dataset.sourceMetadata().sourcePayloadSha256()) {
		issues.emplace_back(
			IssueCode::SourceHashMismatch,
			"Dataset, admission, and ensemble job source hashes differ.");
	}
	if (!admission_report.admitted()) {
		issues.emplace_back(
			IssueCode::AdmissionRejected,
			"Point-cloud admission rejected the segmentation ensemble.");
	}
	if (parameter_sets.empty()) {
		issues.emplace_back(
			IssueCode::NoParameterCandidates,
			"Automated woody segmentation ensemble has no parameter sets.");
	}
	std::set<std::string> parameter_identifiers;
	for (const auto &parameter_set : parameter_sets) {
		if (!parameter_identifiers.insert(
			    parameter_set.parameterIdentifier()).second) {
			issues.emplace_back(
				IssueCode::DuplicateParameterIdentifier,
				"Automated woody segmentation parameter identifiers must be unique.");
			break;
		}
	}
	if (!issues.empty()) return report(std::move(issues));

	std::vector<VegetationWoodySegmentationCandidateEvaluation> evaluations;
	evaluations.reserve(parameter_sets.size());
	for (const auto &parameter_set : parameter_sets) {
		const std::string candidate_identifier =
			ensemble_identifier + ":candidate:" +
			parameter_set.parameterIdentifier();
		auto segmentation_report =
			VegetationWoodyCoverSetSegmentationService().segment(
				candidate_identifier, dataset, admission_report, job,
				parameter_set);
		std::optional<VegetationWoodyBranchGraphReconstructionReport> graph_report;
		double total_axis_length = 0.0;
		double woody_volume = 0.0;
		if (segmentation_report.succeeded()) {
			const VegetationPointCloudReconstructionJob graph_job(
				job_schema,
				job.jobIdentifier() + ":graph:" +
					parameter_set.parameterIdentifier(),
				VegetationPointCloudReconstructionTarget::ShootArchitecture,
				dataset.datasetIdentifier(),
				dataset.sourceMetadata().sourcePayloadSha256(), graph_algorithm,
				job.maximumOutputPrimitives(),
				job.deterministicSeed() ^
					stable_identifier_hash(
						parameter_set.parameterIdentifier()));
			graph_report =
				VegetationSegmentedWoodyBranchGraphReconstructionService()
					.reconstruct(
						ensemble_identifier + ":graph:" +
							parameter_set.parameterIdentifier(),
						dataset, admission_report, graph_job,
						segmentation_report.candidate()->segmentation(),
						graph_policy);
			if (graph_report->succeeded()) {
				const auto dimensions = graph_dimensions(
					*graph_report->reconstruction());
				total_axis_length = dimensions.first;
				woody_volume = dimensions.second;
			}
		}
		evaluations.emplace_back(
			parameter_set, std::move(segmentation_report),
			std::move(graph_report), total_axis_length, woody_volume);
	}
	const auto sensitivity_report =
		VegetationWoodySegmentationSensitivityEvaluationService().evaluate(
			evaluations, sensitivity_policy);
	if (!sensitivity_report.acceptedForGraphSelection()) {
		issues.emplace_back(
			IssueCode::SensitivityGateRejected,
			"Automated woody segmentation sensitivity gate rejected selection.");
		return report(
			std::move(issues), std::move(evaluations), sensitivity_report);
	}
	const std::size_t selected_index =
		*sensitivity_report.selectedCandidateIndex();
	const auto selected_candidate =
		*evaluations[selected_index].segmentationReport().candidate();
	const auto selected_reconstruction =
		*evaluations[selected_index].graphReport()->reconstruction();
	return report(
		{}, std::move(evaluations), sensitivity_report,
		selected_candidate, selected_reconstruction);
}
