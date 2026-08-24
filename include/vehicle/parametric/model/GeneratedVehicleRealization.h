#pragma once

#include "Mesh.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class GeneratedBodyMesh
{
public:
	GeneratedBodyMesh(
		std::shared_ptr<const Mesh> mesh,
		glm::dvec3 bounds_minimum,
		glm::dvec3 bounds_maximum,
		bool watertight,
		std::uint64_t topology_hash)
		: mesh_(std::move(mesh)),
		  bounds_minimum_(bounds_minimum),
		  bounds_maximum_(bounds_maximum),
		  watertight_(watertight),
		  topology_hash_(topology_hash)
	{
	}

	const std::shared_ptr<const Mesh> &mesh() const { return mesh_; }
	const glm::dvec3 &boundsMinimum() const { return bounds_minimum_; }
	const glm::dvec3 &boundsMaximum() const { return bounds_maximum_; }
	glm::dvec3 dimensions() const { return bounds_maximum_ - bounds_minimum_; }
	bool isWatertight() const { return watertight_; }
	std::uint64_t topologyHash() const { return topology_hash_; }

private:
	std::shared_ptr<const Mesh> mesh_;
	glm::dvec3 bounds_minimum_{0.0};
	glm::dvec3 bounds_maximum_{0.0};
	bool watertight_ = false;
	std::uint64_t topology_hash_ = 0u;
};

class GeneratedCharacterCurve
{
public:
	GeneratedCharacterCurve(std::string identifier, std::vector<glm::dvec3> points)
		: identifier_(std::move(identifier)), points_(std::move(points))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<glm::dvec3> &points() const { return points_; }

private:
	std::string identifier_;
	std::vector<glm::dvec3> points_;
};

class GeneratedCharacterCurveSet
{
public:
	explicit GeneratedCharacterCurveSet(std::vector<GeneratedCharacterCurve> curves)
		: curves_(std::move(curves))
	{
	}

	const std::vector<GeneratedCharacterCurve> &curves() const { return curves_; }

private:
	std::vector<GeneratedCharacterCurve> curves_;
};

class GeneratedBodySection
{
public:
	GeneratedBodySection(
		std::string identifier,
		double station,
		std::vector<glm::dvec3> points)
		: identifier_(std::move(identifier)),
		  station_(station),
		  points_(std::move(points))
	{
	}

	const std::string &identifier() const { return identifier_; }
	double station() const { return station_; }
	const std::vector<glm::dvec3> &points() const { return points_; }

private:
	std::string identifier_;
	double station_ = 0.0;
	std::vector<glm::dvec3> points_;
};

class GeneratedBodySectionSet
{
public:
	explicit GeneratedBodySectionSet(std::vector<GeneratedBodySection> sections)
		: sections_(std::move(sections))
	{
	}

	const std::vector<GeneratedBodySection> &sections() const { return sections_; }

private:
	std::vector<GeneratedBodySection> sections_;
};

enum class GeneratedObservationView
{
	Front,
	Rear,
	Left,
	Right,
	Top
};

class GeneratedVehicleObservation
{
public:
	GeneratedVehicleObservation(
		std::string identifier,
		GeneratedObservationView view,
		glm::dvec3 direction,
		glm::dvec3 up,
		std::vector<glm::dvec2> normalized_silhouette)
		: identifier_(std::move(identifier)),
		  view_(view),
		  direction_(direction),
		  up_(up),
		  normalized_silhouette_(std::move(normalized_silhouette))
	{
	}

	const std::string &identifier() const { return identifier_; }
	GeneratedObservationView view() const { return view_; }
	const glm::dvec3 &direction() const { return direction_; }
	const glm::dvec3 &up() const { return up_; }
	const std::vector<glm::dvec2> &normalizedSilhouette() const
	{
		return normalized_silhouette_;
	}

private:
	std::string identifier_;
	GeneratedObservationView view_ = GeneratedObservationView::Front;
	glm::dvec3 direction_{0.0};
	glm::dvec3 up_{0.0};
	std::vector<glm::dvec2> normalized_silhouette_;
};

class GeneratedObservationSet
{
public:
	explicit GeneratedObservationSet(std::vector<GeneratedVehicleObservation> observations)
		: observations_(std::move(observations))
	{
	}

	const std::vector<GeneratedVehicleObservation> &observations() const
	{
		return observations_;
	}

private:
	std::vector<GeneratedVehicleObservation> observations_;
};

class GeneratedArtifactReference
{
public:
	GeneratedArtifactReference(
		std::string semantic_role,
		std::string path,
		std::string sha256)
		: semantic_role_(std::move(semantic_role)),
		  path_(std::move(path)),
		  sha256_(std::move(sha256))
	{
	}

	const std::string &semanticRole() const { return semantic_role_; }
	const std::string &path() const { return path_; }
	const std::string &sha256() const { return sha256_; }

private:
	std::string semantic_role_;
	std::string path_;
	std::string sha256_;
};

class GeneratedArtifactManifest
{
public:
	explicit GeneratedArtifactManifest(std::vector<GeneratedArtifactReference> artifacts)
		: artifacts_(std::move(artifacts))
	{
	}

	const std::vector<GeneratedArtifactReference> &artifacts() const
	{
		return artifacts_;
	}

private:
	std::vector<GeneratedArtifactReference> artifacts_;
};

class GeneratedVehicleRealization
{
public:
	GeneratedVehicleRealization(
		std::string identifier,
		std::string variant_identifier,
		std::uint64_t source_manifest_hash,
		std::uint64_t generation_policy_hash,
		std::uint64_t deterministic_geometry_hash,
		GeneratedBodyMesh body_mesh,
		GeneratedCharacterCurveSet character_curves,
		GeneratedBodySectionSet body_sections,
		GeneratedObservationSet observations,
		GeneratedArtifactManifest artifact_manifest)
		: identifier_(std::move(identifier)),
		  variant_identifier_(std::move(variant_identifier)),
		  source_manifest_hash_(source_manifest_hash),
		  generation_policy_hash_(generation_policy_hash),
		  deterministic_geometry_hash_(deterministic_geometry_hash),
		  body_mesh_(std::move(body_mesh)),
		  character_curves_(std::move(character_curves)),
		  body_sections_(std::move(body_sections)),
		  observations_(std::move(observations)),
		  artifact_manifest_(std::move(artifact_manifest))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &variantIdentifier() const { return variant_identifier_; }
	std::uint64_t sourceManifestHash() const { return source_manifest_hash_; }
	std::uint64_t generationPolicyHash() const { return generation_policy_hash_; }
	std::uint64_t deterministicGeometryHash() const
	{
		return deterministic_geometry_hash_;
	}
	const GeneratedBodyMesh &bodyMesh() const { return body_mesh_; }
	const GeneratedCharacterCurveSet &characterCurves() const
	{
		return character_curves_;
	}
	const GeneratedBodySectionSet &bodySections() const { return body_sections_; }
	const GeneratedObservationSet &observations() const { return observations_; }
	const GeneratedArtifactManifest &artifactManifest() const
	{
		return artifact_manifest_;
	}

private:
	std::string identifier_;
	std::string variant_identifier_;
	std::uint64_t source_manifest_hash_ = 0u;
	std::uint64_t generation_policy_hash_ = 0u;
	std::uint64_t deterministic_geometry_hash_ = 0u;
	GeneratedBodyMesh body_mesh_{nullptr, {}, {}, false, 0u};
	GeneratedCharacterCurveSet character_curves_{{}};
	GeneratedBodySectionSet body_sections_{{}};
	GeneratedObservationSet observations_{{}};
	GeneratedArtifactManifest artifact_manifest_{{}};
};
