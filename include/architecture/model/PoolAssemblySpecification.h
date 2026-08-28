#pragma once

#include <cstddef>
#include <string>
#include <utility>

class PoolAssemblySpecification
{
public:
	PoolAssemblySpecification(
		std::string object_identifier,
		float width,
		float length,
		float depth,
		float shell_thickness,
		float floor_thickness,
		float coping_overhang,
		float coping_thickness,
		float water_freeboard,
		std::size_t step_count,
		std::string shell_material_identifier,
		std::string coping_material_identifier,
		std::string water_material_identifier,
		std::string drain_material_identifier)
		: object_identifier_(std::move(object_identifier)),
		  width_(width),
		  length_(length),
		  depth_(depth),
		  shell_thickness_(shell_thickness),
		  floor_thickness_(floor_thickness),
		  coping_overhang_(coping_overhang),
		  coping_thickness_(coping_thickness),
		  water_freeboard_(water_freeboard),
		  step_count_(step_count),
		  shell_material_identifier_(std::move(shell_material_identifier)),
		  coping_material_identifier_(std::move(coping_material_identifier)),
		  water_material_identifier_(std::move(water_material_identifier)),
		  drain_material_identifier_(std::move(drain_material_identifier))
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float width() const { return width_; }
	float length() const { return length_; }
	float depth() const { return depth_; }
	float shellThickness() const { return shell_thickness_; }
	float floorThickness() const { return floor_thickness_; }
	float copingOverhang() const { return coping_overhang_; }
	float copingThickness() const { return coping_thickness_; }
	float waterFreeboard() const { return water_freeboard_; }
	std::size_t stepCount() const { return step_count_; }
	float interiorWidth() const { return width_ - shell_thickness_ * 2.0f; }
	float interiorLength() const { return length_ - shell_thickness_ * 2.0f; }
	float waterDepth() const
	{
		return depth_ - floor_thickness_ - water_freeboard_;
	}
	const std::string &shellMaterialIdentifier() const
	{
		return shell_material_identifier_;
	}
	const std::string &copingMaterialIdentifier() const
	{
		return coping_material_identifier_;
	}
	const std::string &waterMaterialIdentifier() const
	{
		return water_material_identifier_;
	}
	const std::string &drainMaterialIdentifier() const
	{
		return drain_material_identifier_;
	}

private:
	std::string object_identifier_;
	float width_ = 0.0f;
	float length_ = 0.0f;
	float depth_ = 0.0f;
	float shell_thickness_ = 0.0f;
	float floor_thickness_ = 0.0f;
	float coping_overhang_ = 0.0f;
	float coping_thickness_ = 0.0f;
	float water_freeboard_ = 0.0f;
	std::size_t step_count_ = 0u;
	std::string shell_material_identifier_;
	std::string coping_material_identifier_;
	std::string water_material_identifier_;
	std::string drain_material_identifier_;
};
