#pragma once

#include "editor/relationship/SourceRange.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class SpatialInterfaceInspection
{
public:
	std::string interface_id;
	std::string interface_type;
	std::string region;
	std::string state;
	std::array<float, 3> origin{};
	std::array<float, 3> normal{};
	std::array<float, 3> tangent{};
	float nominal_clearance = 0.0f;
};

class SpatialConnectionInspection
{
public:
	std::string connection_id;
	std::string connection_type;
	std::string source_reference;
	std::string target_reference;
	std::string state;
};

class SpatialConstraintInspection
{
public:
	std::string constraint_id;
	std::string constraint_type;
	std::string source_reference;
	std::string target_reference;
	std::string mode;
	int priority = 0;
};

class SpatialResolutionInspection
{
public:
	std::string constraint_id;
	std::string target_object_id;
	std::string algorithm;
	std::string status;
	int broad_phase_step_count = 0;
	int refinement_iteration_count = 0;
	int collision_query_count = 0;
	float resulting_clearance = 0.0f;
	float residual_error = 0.0f;
	std::array<float, 3> contact_point{};
	std::array<float, 3> contact_normal{};
	std::uint64_t evidence_hash = 0;
};

class SpatialObjectInspection
{
public:
	bool available = false;
	std::string object_id;
	std::string object_name;
	std::string object_class;
	std::string taxonomy_path;
	std::string container_object_id;
	std::string state;
	std::string parent_frame;
	std::string collision_layer;
	std::string collision_mask;
	std::array<float, 16> authored_local_transform{};
	std::array<float, 16> resolution_local_transform{};
	std::array<float, 16> resolved_world_transform{};
	std::array<float, 3> translation_uncertainty{};
	std::array<float, 3> rotation_uncertainty_degrees{};
	std::vector<std::size_t> primitive_instance_indices;
	std::size_t boundary_representation_count = 0;
	std::vector<SpatialInterfaceInspection> interfaces;
	std::vector<SpatialConnectionInspection> connections;
	std::vector<SpatialConstraintInspection> constraints;
	std::vector<SpatialResolutionInspection> resolution_records;
	std::uint64_t aggregate_evidence_hash = 0;
	bool building_knowledge_available = false;
	std::string canonical_concept_id;
	std::string canonical_concept_name;
	std::string semantic_taxonomy_path;
	std::vector<std::string> building_applicability;
	std::vector<std::string> building_roles;
	std::vector<std::string> building_functions;
	std::vector<std::string> building_service_ports;
	std::vector<std::string> incoming_service_flows;
	std::vector<std::string> outgoing_service_flows;
	std::vector<std::string> building_relationship_assertions;
	std::vector<std::string> building_requirements;
	std::vector<std::string> building_scenarios;
	std::vector<std::string> building_evidence;
	std::string placement_state;
	std::string operational_state;
	std::string condition_state;
	std::string compliance_state;
	std::string service_availability_state;
	std::uint64_t semantic_profile_hash = 0;
	std::uint64_t building_model_hash = 0;
	SourceRange source_range;
};

class SpatialObjectSelectionEntry
{
public:
	std::string object_id;
	std::string display_name;
	std::string object_class;
	std::size_t primitive_count = 0;
	int containment_depth = 0;
};
