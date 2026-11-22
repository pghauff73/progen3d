#pragma once

#include "Mesh.h"
#include "geometry/model/FacialSkinLandmarks.h"
#include "geometry/model/MeshSurfaceTag.h"

#include <vector>
#include <string>

// A modest-deformation guard in the neutral head frame. Each moving triangle
// stays within the hemisphere of its own neutral normal. This permits steep
// lip surfaces to rotate across the global +Z plane; it is not a general
// self-contact or large-rotation solver. It never projects candidate geometry.
class FacialSkinAdmissibility
{
public:
	FacialSkinAdmissibility(const Mesh &neutral_mesh,
	                       const std::vector<MeshSurfaceTag> &surface_tags,
	                       const FacialSkinLandmarks &landmarks,
	                       const std::vector<bool> &fixed_vertices,
	                       double characteristic_length);

	bool accepts(const std::vector<glm::dvec3> &positions) const;
	double minimumGlobeClearance(const std::vector<glm::dvec3> &positions) const;
	const std::string &lastRejection() const { return last_rejection_; }

private:
	std::vector<glm::ivec3> faces_;
	std::vector<glm::dvec3> reference_normals_;
	std::vector<bool> moving_faces_;
	std::size_t vertex_count_ = 0;
	glm::dvec3 left_globe_center_{0.0};
	glm::dvec3 right_globe_center_{0.0};
	double globe_radius_ = 0.0;
	double characteristic_length_ = 1.0;
	mutable std::string last_rejection_;
};
