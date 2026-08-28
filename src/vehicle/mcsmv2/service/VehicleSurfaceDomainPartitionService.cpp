#include "vehicle/mcsmv2/service/VehicleSurfaceDomainPartitionService.h"

#include "vehicle/mcsmv2/service/VehicleSurfaceProjectionService.h"
#include "vehicle/mcsmv2/service/VehicleSurfaceDomainEvaluationService.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct DomainCoverageEvidence
{
	double covered_fraction = 0.0;
	std::size_t overlap_sample_count = 0u;
};

DomainCoverageEvidence evaluateDomainCoverage(
	const std::vector<VehicleSurfaceDomain> &domains,
	std::size_t samples)
{
	if (samples == 0u) {
		throw std::invalid_argument("Surface domain coverage requires samples.");
	}
	std::size_t covered_count = 0u;
	std::size_t overlap_count = 0u;
	VehicleSurfaceDomainEvaluationService domain_evaluation;
	for (std::size_t u_index = 0u; u_index < samples; ++u_index) {
		for (std::size_t v_index = 0u; v_index < samples; ++v_index) {
			const glm::dvec2 point(
				(static_cast<double>(u_index) + 0.5) / static_cast<double>(samples),
				(static_cast<double>(v_index) + 0.5) / static_cast<double>(samples));
			std::size_t containing_domain_count = 0u;
			for (const VehicleSurfaceDomain &domain : domains) {
				if (domain_evaluation.contains(domain, point)) ++containing_domain_count;
			}
			if (containing_domain_count > 0u) ++covered_count;
			if (containing_domain_count > 1u) ++overlap_count;
		}
	}
	const double sample_count = static_cast<double>(samples * samples);
	return {
		static_cast<double>(covered_count) / sample_count,
		overlap_count};
}

double cross2D(const glm::dvec2 &first, const glm::dvec2 &second)
{
	return first.x * second.y - first.y * second.x;
}

bool segmentsShareBoundary(
	const glm::dvec2 &first_start,
	const glm::dvec2 &first_end,
	const glm::dvec2 &second_start,
	const glm::dvec2 &second_end)
{
	const glm::dvec2 first_direction = first_end - first_start;
	const glm::dvec2 second_direction = second_end - second_start;
	if (std::fabs(cross2D(first_direction, second_direction)) > 1.0e-10 ||
	    std::fabs(cross2D(second_start - first_start, first_direction)) > 1.0e-10) {
		return false;
	}
	const int dominant_axis =
		std::fabs(first_direction.x) >= std::fabs(first_direction.y) ? 0 : 1;
	const double first_min = std::min(first_start[dominant_axis], first_end[dominant_axis]);
	const double first_max = std::max(first_start[dominant_axis], first_end[dominant_axis]);
	const double second_min =
		std::min(second_start[dominant_axis], second_end[dominant_axis]);
	const double second_max =
		std::max(second_start[dominant_axis], second_end[dominant_axis]);
	return std::min(first_max, second_max) - std::max(first_min, second_min) >
		1.0e-8;
}

bool domainsShareBoundary(
	const VehicleSurfaceDomain &first,
	const VehicleSurfaceDomain &second)
{
	for (const VehicleSurfaceDomainLoop &first_loop : first.loops()) {
		const std::vector<VehicleSurfaceCoordinate> &first_coordinates =
			first_loop.coordinates();
		for (std::size_t first_index = 0u;
		     first_index + 1u < first_coordinates.size();
		     ++first_index) {
			const glm::dvec2 first_start(
				first_coordinates[first_index].longitudinalParameter(),
				first_coordinates[first_index].periodicParameter());
			const glm::dvec2 first_end(
				first_coordinates[first_index + 1u].longitudinalParameter(),
				first_coordinates[first_index + 1u].periodicParameter());
			for (const VehicleSurfaceDomainLoop &second_loop : second.loops()) {
				const std::vector<VehicleSurfaceCoordinate> &second_coordinates =
					second_loop.coordinates();
				for (std::size_t second_index = 0u;
				     second_index + 1u < second_coordinates.size();
				     ++second_index) {
					const glm::dvec2 second_start(
						second_coordinates[second_index].longitudinalParameter(),
						second_coordinates[second_index].periodicParameter());
					const glm::dvec2 second_end(
						second_coordinates[second_index + 1u].longitudinalParameter(),
						second_coordinates[second_index + 1u].periodicParameter());
					if (segmentsShareBoundary(
							first_start, first_end, second_start, second_end)) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

std::vector<VehiclePanelAdjacencyRelationship> derivePanelAdjacencies(
	const std::vector<VehicleSurfaceDomain> &panels)
{
	std::vector<VehiclePanelAdjacencyRelationship> relationships;
	for (std::size_t first_index = 0u; first_index < panels.size(); ++first_index) {
		for (std::size_t second_index = first_index + 1u;
		     second_index < panels.size();
		     ++second_index) {
			if (domainsShareBoundary(panels[first_index], panels[second_index])) {
				relationships.emplace_back(
					panels[first_index].identifier(),
					panels[second_index].identifier(),
					"panel_gap");
			}
		}
	}
	return relationships;
}

void hashOwner(std::uint64_t *hash, const std::string &owner)
{
	for (const unsigned char character : owner) {
		*hash ^= character;
		*hash *= 1099511628211ull;
	}
	*hash ^= 0xffu;
	*hash *= 1099511628211ull;
}

} // namespace

VehicleSurfaceDomainPartition VehicleSurfaceDomainPartitionService::partition(
	const Mesh &final_body_surface,
	const RegisteredVehicleSemanticSurface &registered_surface,
	const VehicleSurfaceDomainCatalog &domain_catalog,
	std::size_t domain_grid_samples,
	double exterior_mapping_limit,
	double aperture_mapping_limit) const
{
	if (!registered_surface.registeredSemanticMesh().mesh() ||
	    final_body_surface.faces.empty()) {
		throw std::invalid_argument(
			"Surface-domain partition requires registered and final body meshes.");
	}
	const DomainCoverageEvidence panel_coverage = evaluateDomainCoverage(
		domain_catalog.panelDomains(), domain_grid_samples);
	const DomainCoverageEvidence aperture_coverage = evaluateDomainCoverage(
		domain_catalog.apertureDomains(), domain_grid_samples);
	const double derived_fixed_body_coverage =
		std::max(0.0, 1.0 - panel_coverage.covered_fraction);
	const double declared_exterior_coverage =
		std::min(1.0, panel_coverage.covered_fraction +
			derived_fixed_body_coverage);

	const Mesh &semantic_mesh =
		*registered_surface.registeredSemanticMesh().mesh();
	const std::vector<glm::dvec2> &semantic_coordinates =
		registered_surface.vertexSurfaceCoordinates();
	VehicleSurfaceProjectionService projection_service;
	VehicleSurfaceDomainEvaluationService domain_evaluation;
	std::vector<std::string> owners;
	owners.reserve(final_body_surface.faces.size());
	std::map<std::string, std::size_t> owner_counts;
	std::uint64_t ownership_hash = 1469598103934665603ull;
	for (const glm::ivec3 &body_face : final_body_surface.faces) {
		const glm::dvec3 body_face_centre =
			(glm::dvec3(final_body_surface.vertices[static_cast<std::size_t>(body_face.x)]) +
			 glm::dvec3(final_body_surface.vertices[static_cast<std::size_t>(body_face.y)]) +
			 glm::dvec3(final_body_surface.vertices[static_cast<std::size_t>(body_face.z)])) /
			3.0;
		const std::optional<VehicleSurfaceProjection> projection =
			projection_service.projectPoint(semantic_mesh, body_face_centre);
		if (!projection) {
			throw std::runtime_error("Final body face could not map to semantic UV.");
		}
		const glm::dvec2 surface_coordinate = domain_evaluation.interpolateFaceCoordinate(
			semantic_mesh.faces[projection->faceIndex()],
			projection->barycentricCoordinates(),
			semantic_coordinates);
		std::string owner = projection->distance() > exterior_mapping_limit
			? "fixed_body_internal"
			: "fixed_body";
		for (const VehicleSurfaceDomain &aperture : domain_catalog.apertureDomains()) {
			if (projection->distance() <= aperture_mapping_limit &&
			    domain_evaluation.contains(aperture, surface_coordinate)) {
				owner = "aperture:" + aperture.identifier();
				break;
			}
		}
		if (owner == "fixed_body") {
			for (const VehicleSurfaceDomain &panel : domain_catalog.panelDomains()) {
				if (domain_evaluation.contains(panel, surface_coordinate)) {
					owner = panel.identifier();
					break;
				}
			}
		}
		owners.push_back(owner);
		++owner_counts[owner];
		hashOwner(&ownership_hash, owner);
	}
	const VehicleSurfaceOwnership ownership(
		std::move(owners),
		std::move(owner_counts),
		0u,
		0u,
		1.0,
		ownership_hash);
	return VehicleSurfaceDomainPartition(
		domain_catalog,
		ownership,
		derivePanelAdjacencies(domain_catalog.panelDomains()),
		panel_coverage.covered_fraction,
		aperture_coverage.covered_fraction,
		derived_fixed_body_coverage,
		declared_exterior_coverage,
		panel_coverage.overlap_sample_count,
		aperture_coverage.overlap_sample_count);
}
