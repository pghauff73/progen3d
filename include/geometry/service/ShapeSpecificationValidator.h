#pragma once

#include "geometry/model/AngularDomainSpecification.h"
#include "geometry/model/AxialDomainSpecification.h"
#include "geometry/model/CylinderShapeSpecification.h"
#include "geometry/model/PolarDomainSpecification.h"
#include "geometry/model/SphereShapeSpecification.h"

#include <memory>
#include <string>
#include <vector>

struct CylinderShapeSpecificationCandidate
{
	float radial_minimum = 0.0f;
	float radial_maximum = 1.0f;
	float axial_minimum = 0.0f;
	float axial_maximum = 1.0f;
	float azimuth_start_degrees = 0.0f;
	float azimuth_sweep_degrees = 360.0f;
	ShapeTopology topology = ShapeTopology::Solid;
	bool topology_was_explicit = false;
	bool radial_was_explicit = false;
	bool wall_was_explicit = false;
	ShapeClosurePolicy closure_policy = ShapeClosurePolicy::createAll();
	int circumferential_segments = 40;
	int axial_segments = 1;
	ShapeMappingMode mapping_mode = ShapeMappingMode::Triplanar;
	std::vector<PlaneClipSpecification> clips;
};

struct SphereShapeSpecificationCandidate
{
	float radial_minimum = 0.0f;
	float radial_maximum = 1.0f;
	float polar_minimum_degrees = 0.0f;
	float polar_maximum_degrees = 180.0f;
	float azimuth_start_degrees = 0.0f;
	float azimuth_sweep_degrees = 360.0f;
	ShapeTopology topology = ShapeTopology::Solid;
	bool topology_was_explicit = false;
	bool radial_was_explicit = false;
	bool wall_was_explicit = false;
	ShapeClosurePolicy closure_policy = ShapeClosurePolicy::createAll();
	int azimuth_segments = 40;
	int polar_segments = 20;
	ShapeMappingMode mapping_mode = ShapeMappingMode::Triplanar;
	std::vector<PlaneClipSpecification> clips;
};

class ShapeSpecificationValidator
{
public:
	std::shared_ptr<const CylinderShapeSpecification> validateCylinder(
		CylinderShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

	std::shared_ptr<const SphereShapeSpecification> validateSphere(
		SphereShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;
};
