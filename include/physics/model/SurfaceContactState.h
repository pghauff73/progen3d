#pragma once

#include <glm/glm.hpp>

// SI units. The two material layers act as springs in series at one contact patch.
struct SurfaceContactMaterial {
	bool enabled = false;
	glm::dvec3 shaft_axis{0.0};          // world axis through centre of mass
	bool onShaft() const { return glm::dot(shaft_axis, shaft_axis) > 0.5; }
	double normal_stiffness = 10000.0;    // N/m
	double shear_stiffness = 2500.0;      // N/m
	double friction = 0.4;
	double initial_temperature = 293.15; // K
	double specific_heat = 500.0;         // J/(kg K)
	double young_modulus = 0.0;           // Pa, optional homogeneous isotropic layer
	double poisson_ratio = 0.0;
	double patch_area = 0.0;              // m^2, prescribed effective patch
	double layer_thickness = 0.0;         // m
	double conductance = 0.0;             // W/K per material layer
};

struct SurfaceBodyState {
	glm::dmat3 inertia{0.0};             // world axes, about centre of mass
	glm::dmat3 initial_inertia{0.0};
	double volume = 0.0;
	double step_limit = 1.0 / 480.0;
	double temperature = 293.15;
	double friction_heat = 0.0;           // cumulative generated heat received
	double conducted_heat = 0.0;          // cumulative net received, signed
	glm::dvec3 force{0.0};                // sum over current contacts
	glm::dvec3 torque{0.0};
	double elastic_energy = 0.0;          // this body's share
	double normal_compression = 0.0;      // largest local layer deformation
	double shear_displacement = 0.0;

	void clearContactLoads() {
		force = torque = glm::dvec3(0.0);
		elastic_energy = normal_compression = shear_displacement = 0.0;
	}
};

struct SurfaceContactPatch {
	glm::dvec3 normal{0.0};
	glm::dvec3 elastic_shear{0.0};
	double shear_energy = 0.0;
};

// Snapshot returned to the inspector or to a numerical experiment.
struct SurfaceMotionBalance {
	glm::dvec3 linear_momentum{0.0};
	glm::dvec3 spin_momentum{0.0};
	double translation_energy = 0.0;
	double rotation_energy = 0.0;
	double gravity_potential = 0.0;
};
