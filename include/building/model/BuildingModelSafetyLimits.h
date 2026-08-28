#pragma once

#include <cstddef>

class BuildingModelSafetyLimits {
public:
	std::size_t maximum_concepts = 512;
	std::size_t maximum_roles = 1024;
	std::size_t maximum_profiles = 4096;
	std::size_t maximum_functions = 1024;
	std::size_t maximum_allocations = 8192;
	std::size_t maximum_function_dependencies = 4096;
	std::size_t maximum_service_systems = 256;
	std::size_t maximum_service_ports = 8192;
	std::size_t maximum_service_flows = 16384;
	std::size_t maximum_relationship_assertions = 4096;
	std::size_t maximum_requirements = 32768;
	std::size_t maximum_requirement_dependency_depth = 256;
	std::size_t maximum_scenarios = 512;
	std::size_t maximum_evidence_records = 65536;
	std::size_t maximum_diagnostic_count = 4096;
	std::size_t maximum_source_string_length = 16384;
};
