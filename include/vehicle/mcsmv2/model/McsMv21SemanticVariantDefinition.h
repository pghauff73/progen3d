#pragma once

#include "vehicle/mcsmv2/model/SemanticImplicitCorrespondenceReport.h"
#include "vehicle/mcsmv2/model/VehicleSurfaceDomainCatalog.h"

#include <cstddef>
#include <string>
#include <utility>

class McsMv21SemanticVariantDefinition
{
public:
	McsMv21SemanticVariantDefinition(
		std::string identifier,
		std::string display_name,
		VehicleSurfaceDomainCatalog domain_catalog,
		SemanticImplicitCorrespondenceReport accepted_correspondence,
		std::size_t registered_surface_vertex_count,
		std::size_t registered_surface_face_count,
		std::size_t final_body_vertex_count,
		std::size_t final_body_face_count,
		bool source_release_gate_pass,
		std::string assurance_level)
		: identifier_(std::move(identifier)),
		  display_name_(std::move(display_name)),
		  domain_catalog_(std::move(domain_catalog)),
		  accepted_correspondence_(std::move(accepted_correspondence)),
		  registered_surface_vertex_count_(registered_surface_vertex_count),
		  registered_surface_face_count_(registered_surface_face_count),
		  final_body_vertex_count_(final_body_vertex_count),
		  final_body_face_count_(final_body_face_count),
		  source_release_gate_pass_(source_release_gate_pass),
		  assurance_level_(std::move(assurance_level))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &displayName() const { return display_name_; }
	const VehicleSurfaceDomainCatalog &domainCatalog() const { return domain_catalog_; }
	const SemanticImplicitCorrespondenceReport &acceptedCorrespondence() const
	{
		return accepted_correspondence_;
	}
	std::size_t registeredSurfaceVertexCount() const
	{
		return registered_surface_vertex_count_;
	}
	std::size_t registeredSurfaceFaceCount() const { return registered_surface_face_count_; }
	std::size_t finalBodyVertexCount() const { return final_body_vertex_count_; }
	std::size_t finalBodyFaceCount() const { return final_body_face_count_; }
	bool sourceReleaseGatePass() const { return source_release_gate_pass_; }
	const std::string &assuranceLevel() const { return assurance_level_; }

private:
	std::string identifier_;
	std::string display_name_;
	VehicleSurfaceDomainCatalog domain_catalog_{"", {}, {}};
	SemanticImplicitCorrespondenceReport accepted_correspondence_{
		SurfaceCorrespondenceDirectionReport(0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, ""),
		SurfaceCorrespondenceDirectionReport(0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, ""),
		0.0, 0.0, 0.0, {}, false};
	std::size_t registered_surface_vertex_count_ = 0u;
	std::size_t registered_surface_face_count_ = 0u;
	std::size_t final_body_vertex_count_ = 0u;
	std::size_t final_body_face_count_ = 0u;
	bool source_release_gate_pass_ = false;
	std::string assurance_level_;
};
