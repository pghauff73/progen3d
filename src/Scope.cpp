#include "Scope.h"

#include <cmath>

#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {

constexpr float kBasisEpsilon = 1.0e-7f;

float normalize_degrees(float angle)
{
	angle = std::fmod(angle, 360.0f);
	if (angle < 0.0f) {
		angle += 360.0f;
	}
	return angle;
}

glm::vec3 normalized_or(const glm::vec3 &value, const glm::vec3 &fallback)
{
	const float squared_length = glm::dot(value, value);
	if (!std::isfinite(squared_length) || squared_length <= kBasisEpsilon) {
		return fallback;
	}
	return value / std::sqrt(squared_length);
}

glm::vec3 perpendicular_to(const glm::vec3 &axis)
{
	const glm::vec3 candidate =
		std::fabs(axis.x) < 0.75f ? glm::vec3(1.0f, 0.0f, 0.0f)
		                              : glm::vec3(0.0f, 1.0f, 0.0f);
	return normalized_or(glm::cross(axis, candidate), glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::vec3 matrix_scale_magnitudes(const glm::mat4 &transform)
{
	return glm::vec3(glm::length(glm::vec3(transform[0])),
	                 glm::length(glm::vec3(transform[1])),
	                 glm::length(glm::vec3(transform[2])));
}

void synchronize_metadata_from_transforms(const glm::mat4 &primary,
                                          const glm::mat4 &secondary,
                                          glm::vec3 *position,
                                          glm::vec3 *primary_size,
                                          glm::vec3 *secondary_size,
                                          glm::vec3 *basis_x,
                                          glm::vec3 *basis_y,
                                          glm::vec3 *basis_z,
                                          float *angle_x,
                                          float *angle_y,
                                          float *angle_z)
{
	if (position == nullptr || primary_size == nullptr || secondary_size == nullptr ||
	    basis_x == nullptr || basis_y == nullptr || basis_z == nullptr ||
	    angle_x == nullptr || angle_y == nullptr || angle_z == nullptr) {
		return;
	}

	*position = glm::vec3(primary[3]);
	*primary_size = matrix_scale_magnitudes(primary);
	*secondary_size = matrix_scale_magnitudes(secondary);

	const glm::vec3 raw_x(primary[0]);
	const glm::vec3 raw_y(primary[1]);
	const glm::vec3 raw_z(primary[2]);
	glm::vec3 x_axis = normalized_or(raw_x, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::vec3 y_rejected = raw_y - x_axis * glm::dot(raw_y, x_axis);
	if (glm::dot(y_rejected, y_rejected) <= kBasisEpsilon) {
		y_rejected = perpendicular_to(x_axis);
	}
	glm::vec3 y_axis = normalized_or(y_rejected, perpendicular_to(x_axis));
	glm::vec3 z_axis = normalized_or(glm::cross(x_axis, y_axis),
	                                 glm::vec3(0.0f, 0.0f, 1.0f));
	if (glm::dot(z_axis, raw_z) < 0.0f) {
		z_axis = -z_axis;
	}
	y_axis = normalized_or(glm::cross(z_axis, x_axis), y_axis);

	*basis_x = x_axis;
	*basis_y = y_axis;
	*basis_z = z_axis;

	const glm::mat3 rotation(x_axis, y_axis, z_axis);
	glm::quat orientation = glm::quat_cast(rotation);
	const float quaternion_length = glm::length(orientation);
	if (!std::isfinite(quaternion_length) || quaternion_length <= kBasisEpsilon) {
		orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	} else {
		orientation /= quaternion_length;
	}
	const glm::vec3 euler_degrees = glm::degrees(glm::eulerAngles(orientation));
	*angle_x = normalize_degrees(euler_degrees.x);
	*angle_y = normalize_degrees(euler_degrees.y);
	*angle_z = normalize_degrees(euler_degrees.z);
}

} // namespace

SpatialTransformScope::SpatialTransformScope()
{
	Transform = glm::mat4(1.0f);
	Transform2 = glm::mat4(1.0f);
	synchronize_metadata_from_transforms(Transform,
	                                     Transform2,
	                                     &position,
	                                     &size,
	                                     &size2,
	                                     &x,
	                                     &y,
	                                     &z,
	                                     &anglex,
	                                     &angley,
	                                     &anglez);
}

SpatialTransformScope::SpatialTransformScope(SpatialTransformScope *other)
{
	if (other == nullptr) {
		Transform = glm::mat4(1.0f);
		Transform2 = glm::mat4(1.0f);
	} else {
		dual_scales = other->dual_scales;
		dual_translations = other->dual_translations;
		Transform = other->Transform;
		Transform2 = other->Transform2;
	}
	synchronize_metadata_from_transforms(Transform,
	                                     Transform2,
	                                     &position,
	                                     &size,
	                                     &size2,
	                                     &x,
	                                     &y,
	                                     &z,
	                                     &anglex,
	                                     &angley,
	                                     &anglez);
}

void SpatialTransformScope::translate(const glm::vec3 &translation)
{
	const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), translation);
	Transform = Transform * translation_matrix;
	Transform2 = Transform2 * translation_matrix;
	synchronize_metadata_from_transforms(Transform, Transform2, &position, &size, &size2,
	                                     &x, &y, &z, &anglex, &angley, &anglez);
}

void SpatialTransformScope::scalePrimary(const glm::vec3 &size_)
{
	const glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), size_);
	Transform = Transform * scale_matrix;
	Transform2 = Transform2 * scale_matrix;
	synchronize_metadata_from_transforms(Transform, Transform2, &position, &size, &size2,
	                                     &x, &y, &z, &anglex, &angley, &anglez);
}

void SpatialTransformScope::scaleSecondary(const glm::vec3 &size_)
{
	Transform2 = Transform2 * glm::scale(glm::mat4(1.0f), size_);
	synchronize_metadata_from_transforms(Transform, Transform2, &position, &size, &size2,
	                                     &x, &y, &z, &anglex, &angley, &anglez);
}

void SpatialTransformScope::scaleDualAxis(int axis, const glm::vec3 &size_)
{
	if (axis < 0 || axis >= static_cast<int>(dual_scales.size())) {
		return;
	}
	dual_scales[static_cast<std::size_t>(axis)] *= size_;
}

void SpatialTransformScope::translateDualAxis(int axis, const glm::vec3 &translation)
{
	if (axis < 0 || axis >= static_cast<int>(dual_translations.size())) {
		return;
	}
	dual_translations[static_cast<std::size_t>(axis)] += translation;
}

void SpatialTransformScope::rotateAroundX(float angle)
{
	const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f),
	                                       glm::radians(angle),
	                                       glm::vec3(1.0f, 0.0f, 0.0f));
	Transform = Transform * rotation;
	Transform2 = Transform2 * rotation;
	synchronize_metadata_from_transforms(Transform, Transform2, &position, &size, &size2,
	                                     &x, &y, &z, &anglex, &angley, &anglez);
}

void SpatialTransformScope::rotateAroundY(float angle)
{
	const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f),
	                                       glm::radians(angle),
	                                       glm::vec3(0.0f, 1.0f, 0.0f));
	Transform = Transform * rotation;
	Transform2 = Transform2 * rotation;
	synchronize_metadata_from_transforms(Transform, Transform2, &position, &size, &size2,
	                                     &x, &y, &z, &anglex, &angley, &anglez);
}

void SpatialTransformScope::rotateAroundZ(float angle)
{
	const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f),
	                                       glm::radians(angle),
	                                       glm::vec3(0.0f, 0.0f, 1.0f));
	Transform = Transform * rotation;
	Transform2 = Transform2 * rotation;
	synchronize_metadata_from_transforms(Transform, Transform2, &position, &size, &size2,
	                                     &x, &y, &z, &anglex, &angley, &anglez);
}

void SpatialTransformScope::T(const glm::vec3 &translation)
{
	translate(translation);
}

void SpatialTransformScope::S(const glm::vec3 &size_)
{
	scalePrimary(size_);
}

void SpatialTransformScope::D(const glm::vec3 &size_)
{
	scaleSecondary(size_);
}

void SpatialTransformScope::DS(int axis, const glm::vec3 &size_)
{
	scaleDualAxis(axis, size_);
}

void SpatialTransformScope::DT(int axis, const glm::vec3 &translation)
{
	translateDualAxis(axis, translation);
}

void SpatialTransformScope::Rx(float angle)
{
	rotateAroundX(angle);
}

void SpatialTransformScope::Ry(float angle)
{
	rotateAroundY(angle);
}

void SpatialTransformScope::Rz(float angle)
{
	rotateAroundZ(angle);
}

const glm::mat4 &SpatialTransformScope::getTransform() const
{
	return Transform;
}

const glm::mat4 &SpatialTransformScope::getTransform2() const
{
	return Transform2;
}

const std::array<glm::vec3, 3> &SpatialTransformScope::getDualScales() const
{
	return dual_scales;
}

const std::array<glm::vec3, 3> &SpatialTransformScope::getDualTranslations() const
{
	return dual_translations;
}

const glm::vec3 &SpatialTransformScope::getPosition() const
{
	return position;
}

glm::vec3 SpatialTransformScope::setPosition(glm::vec3 pos)
{
	Transform[3] = glm::vec4(pos, 1.0f);
	Transform2[3] = glm::vec4(pos, 1.0f);
	synchronize_metadata_from_transforms(Transform, Transform2, &position, &size, &size2,
	                                     &x, &y, &z, &anglex, &angley, &anglez);
	return position;
}

const glm::vec3 &SpatialTransformScope::getSize() const
{
	return size;
}

const glm::vec3 &SpatialTransformScope::getSize2() const
{
	return size2;
}

glm::vec3 SpatialTransformScope::getEulerDegrees() const
{
	return glm::vec3(anglex, angley, anglez);
}
