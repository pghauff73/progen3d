#pragma once

#include <cstddef>

class SpatialModelSafetyLimits {
public:
	std::size_t maximum_objects = 4096;
	std::size_t maximum_interfaces_per_object = 64;
	std::size_t maximum_connections = 8192;
	std::size_t maximum_constraints = 8192;
	std::size_t maximum_primitive_bindings_per_object = 1024;
	std::size_t maximum_containment_depth = 128;
	std::size_t maximum_constraint_dependency_depth = 512;
	std::size_t maximum_resolution_records_per_object = 256;
	std::size_t maximum_cycle_diagnostic_length = 512;
};
