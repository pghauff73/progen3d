#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

class ConvexHullMeshGenerationReport
{
public:
	ConvexHullMeshGenerationReport(
		std::string algorithm,
		std::size_t source_point_count,
		std::size_t hull_vertex_count,
		std::size_t hull_face_count,
		double volume,
		bool contains_all_source_points,
		bool topology_passed)
		: algorithm_(std::move(algorithm)),
		  source_point_count_(source_point_count),
		  hull_vertex_count_(hull_vertex_count),
		  hull_face_count_(hull_face_count),
		  volume_(volume),
		  contains_all_source_points_(contains_all_source_points),
		  topology_passed_(topology_passed)
	{
	}

	const std::string &algorithm() const { return algorithm_; }
	std::size_t sourcePointCount() const { return source_point_count_; }
	std::size_t hullVertexCount() const { return hull_vertex_count_; }
	std::size_t hullFaceCount() const { return hull_face_count_; }
	double volume() const { return volume_; }
	bool containsAllSourcePoints() const { return contains_all_source_points_; }
	bool topologyPassed() const { return topology_passed_; }
	bool passed() const
	{
		return contains_all_source_points_ && topology_passed_ && volume_ > 0.0;
	}

private:
	std::string algorithm_;
	std::size_t source_point_count_ = 0u;
	std::size_t hull_vertex_count_ = 0u;
	std::size_t hull_face_count_ = 0u;
	double volume_ = 0.0;
	bool contains_all_source_points_ = false;
	bool topology_passed_ = false;
};

class ConvexHullMeshGenerationResult
{
public:
	ConvexHullMeshGenerationResult(
		GeneratedPrimitiveMesh hull_mesh,
		ConvexHullMeshGenerationReport report)
		: hull_mesh_(std::move(hull_mesh)), report_(std::move(report))
	{
	}

	const GeneratedPrimitiveMesh &hullMesh() const { return hull_mesh_; }
	const ConvexHullMeshGenerationReport &report() const { return report_; }

private:
	GeneratedPrimitiveMesh hull_mesh_{std::make_shared<Mesh>(), {}};
	ConvexHullMeshGenerationReport report_{"", 0u, 0u, 0u, 0.0, false, false};
};
