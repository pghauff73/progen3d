#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cmath>
#include <string>
#include <utility>

class VehicleReferenceFrameDefinition
{
public:
	using TransformationMatrix = std::array<std::array<double, 4>, 4>;

	VehicleReferenceFrameDefinition(
		std::string schema,
		std::string identifier,
		glm::dvec3 source_origin,
		glm::dvec3 local_source_origin,
		glm::dvec3 source_x_axis,
		glm::dvec3 source_y_axis,
		glm::dvec3 source_z_axis,
		std::string handedness,
		std::string length_unit,
		std::string angle_unit,
		TransformationMatrix source_to_progen3d_matrix,
		double progen3d_ground_plane_height)
		: schema_(std::move(schema)),
		  identifier_(std::move(identifier)),
		  source_origin_(source_origin),
		  local_source_origin_(local_source_origin),
		  source_x_axis_(source_x_axis),
		  source_y_axis_(source_y_axis),
		  source_z_axis_(source_z_axis),
		  handedness_(std::move(handedness)),
		  length_unit_(std::move(length_unit)),
		  angle_unit_(std::move(angle_unit)),
		  source_to_progen3d_matrix_(source_to_progen3d_matrix),
		  progen3d_ground_plane_height_(progen3d_ground_plane_height)
	{
	}

	const std::string &schema() const { return schema_; }
	const std::string &identifier() const { return identifier_; }
	const glm::dvec3 &sourceOrigin() const { return source_origin_; }
	const glm::dvec3 &localSourceOrigin() const { return local_source_origin_; }
	const glm::dvec3 &sourceXAxis() const { return source_x_axis_; }
	const glm::dvec3 &sourceYAxis() const { return source_y_axis_; }
	const glm::dvec3 &sourceZAxis() const { return source_z_axis_; }
	const std::string &handedness() const { return handedness_; }
	const std::string &lengthUnit() const { return length_unit_; }
	const std::string &angleUnit() const { return angle_unit_; }
	const TransformationMatrix &sourceToProgen3dMatrix() const
	{
		return source_to_progen3d_matrix_;
	}
	double progen3dGroundPlaneHeight() const
	{
		return progen3d_ground_plane_height_;
	}

	glm::dvec3 convertSourcePointToProgen3d(const glm::dvec3 &source_point) const
	{
		return transform(source_point - local_source_origin_, 1.0);
	}

	glm::dvec3 convertSourceDirectionToProgen3d(
		const glm::dvec3 &source_direction) const
	{
		return transform(source_direction, 0.0);
	}

	glm::dvec3 convertProgen3dPointToSource(
		const glm::dvec3 &progen3d_point) const
	{
		return inverseTransform(progen3d_point, true) + local_source_origin_;
	}

	glm::dvec3 convertProgen3dDirectionToSource(
		const glm::dvec3 &progen3d_direction) const
	{
		return inverseTransform(progen3d_direction, false);
	}

	glm::dvec3 progen3dForwardDirection() const
	{
		return convertSourceDirectionToProgen3d(source_x_axis_);
	}

	glm::dvec3 progen3dRightDirection() const
	{
		return convertSourceDirectionToProgen3d(source_y_axis_);
	}

	glm::dvec3 progen3dUpDirection() const
	{
		return convertSourceDirectionToProgen3d(source_z_axis_);
	}

	bool isFinite() const
	{
		for (const auto &row : source_to_progen3d_matrix_) {
			for (double value : row) {
				if (!std::isfinite(value)) return false;
			}
		}
		return finite(source_origin_) && finite(local_source_origin_) &&
			finite(source_x_axis_) && finite(source_y_axis_) && finite(source_z_axis_) &&
			std::isfinite(progen3d_ground_plane_height_);
	}

private:
	static bool finite(const glm::dvec3 &value)
	{
		return std::isfinite(value.x) && std::isfinite(value.y) &&
			std::isfinite(value.z);
	}

	glm::dvec3 transform(const glm::dvec3 &value, double homogeneous_w) const
	{
		const std::array<double, 4> input{{value.x, value.y, value.z, homogeneous_w}};
		glm::dvec3 result(0.0);
		for (std::size_t row = 0u; row < 3u; ++row) {
			double component = 0.0;
			for (std::size_t column = 0u; column < 4u; ++column) {
				component += source_to_progen3d_matrix_[row][column] * input[column];
			}
			result[static_cast<int>(row)] = component;
		}
		return result;
	}

	glm::dvec3 inverseTransform(
		const glm::dvec3 &value,
		bool include_translation) const
	{
		glm::dmat3 linear(1.0);
		for (std::size_t row = 0u; row < 3u; ++row) {
			for (std::size_t column = 0u; column < 3u; ++column) {
				linear[static_cast<int>(column)][static_cast<int>(row)] =
					source_to_progen3d_matrix_[row][column];
			}
		}
		glm::dvec3 adjusted = value;
		if (include_translation) {
			adjusted -= glm::dvec3(
				source_to_progen3d_matrix_[0][3],
				source_to_progen3d_matrix_[1][3],
				source_to_progen3d_matrix_[2][3]);
		}
		return glm::inverse(linear) * adjusted;
	}

	std::string schema_;
	std::string identifier_;
	glm::dvec3 source_origin_{0.0};
	glm::dvec3 local_source_origin_{0.0};
	glm::dvec3 source_x_axis_{1.0, 0.0, 0.0};
	glm::dvec3 source_y_axis_{0.0, 1.0, 0.0};
	glm::dvec3 source_z_axis_{0.0, 0.0, 1.0};
	std::string handedness_;
	std::string length_unit_;
	std::string angle_unit_;
	TransformationMatrix source_to_progen3d_matrix_{{}};
	double progen3d_ground_plane_height_ = 0.0;
};
