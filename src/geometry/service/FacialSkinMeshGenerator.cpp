#include "geometry/service/FacialSkinMeshGenerator.h"

#include "geometry/model/FacialSkinShapeSpecification.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace {

float smooth_transition(float minimum, float maximum, float value)
{
	const float fraction = std::clamp((value - minimum) / (maximum - minimum), 0.0f, 1.0f);
	return fraction * fraction * (3.0f - 2.0f * fraction);
}

float localized_field(float u, float v, float center_u, float center_v,
	                  float radius_u, float radius_v)
{
	const float horizontal = (u - center_u) / radius_u;
	const float vertical = (v - center_v) / radius_v;
	return std::exp(-horizontal * horizontal - vertical * vertical);
}

enum class FacialAperturePurpose { LeftEye, RightEye, Mouth };

struct FacialApertureDomain
{
	FacialAperturePurpose purpose;
	int first_column;
	int last_column;
	int first_row;
	int last_row;
	float center_u;
	float center_v;
	float radius_u;
	float radius_v;

	bool containsInteriorVertex(int column, int row) const
	{
		return column > first_column && column < last_column &&
		       row > first_row && row < last_row;
	}

	bool containsCell(int column, int row) const
	{
		return column >= first_column && column < last_column &&
		       row >= first_row && row < last_row;
	}
};

class FacialSkinSurfaceEvaluation
{
public:
	explicit FacialSkinSurfaceEvaluation(const FacialSkinShapeSpecification &specification)
		: specification_(specification)
	{
		const int horizontal_scale = specification.horizontalSegments() / 48;
		const int vertical_scale = specification.verticalSegments() / 64;
		apertures_ = {{
			{FacialAperturePurpose::LeftEye, 28 * horizontal_scale, 40 * horizontal_scale,
			 38 * vertical_scale, 46 * vertical_scale, 5.0f / 12.0f, 5.0f / 16.0f, 0.25f, 0.125f},
			{FacialAperturePurpose::RightEye, 8 * horizontal_scale, 20 * horizontal_scale,
			 38 * vertical_scale, 46 * vertical_scale, -5.0f / 12.0f, 5.0f / 16.0f, 0.25f, 0.125f},
			{FacialAperturePurpose::Mouth, 14 * horizontal_scale, 34 * horizontal_scale,
			 16 * vertical_scale, 24 * vertical_scale, 0.0f, -0.375f, 5.0f / 12.0f, 0.125f}
		}};
	}

	const std::array<FacialApertureDomain, 3> &apertures() const { return apertures_; }

	float eyeballRadius() const
	{
		const glm::vec3 &size = specification_.size();
		return 1.2f * std::max(0.5f * size.x * 0.25f * 0.86f,
		                       0.5f * size.y * 0.125f * 0.70f);
	}

	glm::vec3 eyeballCenter(const FacialApertureDomain &aperture) const
	{
		return glm::vec3(0.5f * specification_.size().x * aperture.center_u,
		                 0.5f * specification_.size().y * aperture.center_v,
		                 restDepth(aperture.center_u, aperture.center_v) - 0.55f * eyeballRadius());
	}

	glm::vec3 evaluatePosition(float rest_u, float rest_v) const
	{
		const glm::vec3 &size = specification_.size();
		const FacialExpressionControls &controls = specification_.controls();
		float surface_u = rest_u;
		float surface_v = rest_v;
		float aperture_weight = 0.0f;
		const FacialApertureDomain *active_aperture = nullptr;
		for (const FacialApertureDomain &aperture : apertures_) {
			const float local_u = (rest_u - aperture.center_u) / aperture.radius_u;
			const float local_v = (rest_v - aperture.center_v) / aperture.radius_v;
			const float square_radius = std::max(std::fabs(local_u), std::fabs(local_v));
			if (square_radius < 0.9999f || square_radius >= 1.65f) continue;
			const float radial_length = std::sqrt(local_u * local_u + local_v * local_v);
			const float circle_u = local_u / radial_length;
			const float circle_v = local_v / radial_length;
			aperture_weight = 1.0f - smooth_transition(1.0f, 1.65f, square_radius);
			active_aperture = &aperture;
			const float aperture_u = aperture.center_u + aperture.radius_u * 0.86f * circle_u;
			float aperture_v = aperture.center_v;
			if (aperture.purpose == FacialAperturePurpose::Mouth) {
				// A small contact clearance preserves topology at rest. Upper lip
				// motion is restrained while the lower aperture follows jaw opening.
				aperture_v -= 0.060f * controls.jaw_open;
				aperture_v += (0.002f + 0.070f * controls.jaw_open) * circle_v;
			} else {
				const float blink = aperture.purpose == FacialAperturePurpose::LeftEye
					? controls.blink_left : controls.blink_right;
				const float half_gap = aperture.radius_v * 0.70f * (1.0f - blink) + 0.0008f * blink;
				aperture_v += half_gap * circle_v;
			}
			// Interpolate the annulus between its elliptical inner boundary and
			// rectangular outer boundary. Positive radial progress is retained
			// even when a blink almost closes the inner aperture.
			const float annulus_fraction = (square_radius - 1.0f) / 0.65f;
			const float outer_u = aperture.center_u + 1.65f * aperture.radius_u * local_u / square_radius;
			const float outer_v = aperture.center_v + 1.65f * aperture.radius_v * local_v / square_radius;
			surface_u = aperture_u * (1.0f - annulus_fraction) + outer_u * annulus_fraction;
			surface_v = aperture_v * (1.0f - annulus_fraction) + outer_v * annulus_fraction;
			break;
		}

		// Preserve the feature chart, then map the outer square annulus onto an
		// rounded contour by monotone radial interpolation. A full square-to-disc map would
		// curve a nearly closed eyelid more sharply than its coarse triangles.
		const float square_radius = std::max(std::fabs(surface_u), std::fabs(surface_v));
		if (square_radius > 0.68f) {
			const float direction_u = surface_u / square_radius;
			const float direction_v = surface_v / square_radius;
			const float direction_length = std::pow(
				std::pow(direction_u, 4.0f) + std::pow(direction_v, 4.0f), 0.25f);
			const float fraction = (square_radius - 0.68f) / 0.32f;
			const float scale = 0.68f * (1.0f - fraction) + fraction / direction_length;
			surface_u = direction_u * scale;
			surface_v = direction_v * scale;
		}

		float depth = restDepth(rest_u, rest_v);
		float expression_weight = 1.0f;
		if (active_aperture != nullptr && active_aperture->purpose != FacialAperturePurpose::Mouth) {
			const float blink = active_aperture->purpose == FacialAperturePurpose::LeftEye
				? controls.blink_left : controls.blink_right;
			// Blink closure takes priority over overlaid cheek/brow motion at the
			// margins while the surrounding facial tissue still follows expression.
			expression_weight -= blink * aperture_weight;
		}
		for (int side : {-1, 1}) {
			const float smile = side > 0 ? controls.smile_left : controls.smile_right;
			const float brow = side > 0 ? controls.brow_raise_left : controls.brow_raise_right;
			const float smile_weight = localized_field(rest_u, rest_v,
				side * 0.40f, -0.28f, 0.27f, 0.24f);
			const float brow_weight = localized_field(rest_u, rest_v,
				side * 0.40f, 0.57f, 0.25f, 0.12f);
			surface_u += expression_weight * side * 0.045f * smile * smile_weight;
			surface_v += expression_weight * (0.058f * smile * smile_weight + 0.055f * brow * brow_weight);
			depth += expression_weight * size.z * 0.025f * smile * smile_weight;
		}
		surface_v -= 0.12f * controls.jaw_open * smooth_transition(0.50f, 0.90f, -rest_v);
		glm::vec3 position(0.5f * size.x * surface_u, 0.5f * size.y * surface_v, depth);
		if (active_aperture != nullptr) {
			if (active_aperture->purpose == FacialAperturePurpose::Mouth) {
				position.z += size.z * 0.055f * aperture_weight;
			} else {
				const glm::vec3 center = eyeballCenter(*active_aperture);
				const float radius = eyeballRadius();
				const float horizontal = position.x - center.x;
				const float vertical = position.y - center.y;
				const float sphere_depth = center.z + std::sqrt(std::max(0.0f,
					radius * radius - horizontal * horizontal - vertical * vertical));
				const float clearance = size.z * 0.006f;
				position.z += aperture_weight * (sphere_depth + clearance - position.z);
			}
		}
		// A blended target alone can leave the surrounding eyelid under the
		// eyeball. Apply a final one-sided geometric clearance to all skin points
		// in either globe's projected footprint, including the aperture collar.
		for (const auto &aperture : apertures_) {
			if (aperture.purpose == FacialAperturePurpose::Mouth) continue;
			const glm::vec3 center = eyeballCenter(aperture);
			const float radius = eyeballRadius();
			const float local_radius = std::max(
				std::fabs((rest_u - aperture.center_u) / aperture.radius_u),
				std::fabs((rest_v - aperture.center_v) / aperture.radius_v));
			// The collar reserves an envelope outside the globe for straight
			// triangle chords near its silhouette. Testing vertex clearance alone
			// misses those intersections. The aperture itself still hugs the globe.
			const float guard_radius = radius *
				(1.0f + 0.16f * smooth_transition(1.0f, 1.25f, local_radius));
			const float horizontal = position.x - center.x;
			const float vertical = position.y - center.y;
			const float squared_depth = guard_radius * guard_radius - horizontal * horizontal - vertical * vertical;
			if (squared_depth <= 0.0f) continue;
			position.z = std::max(position.z,
				center.z + std::sqrt(squared_depth) + size.z * 0.006f);
		}
		return position;
	}

private:
	float restDepth(float u, float v) const
	{
		const float dome = 0.48f + 0.27f * (1.0f - u * u) + 0.18f * (1.0f - v * v);
		const float nose = 0.34f * localized_field(u, v, 0.0f, -0.035f, 0.13f, 0.16f);
		const float bridge = 0.12f * localized_field(u, v, 0.0f, 0.19f, 0.095f, 0.31f);
		const float chin = 0.055f * localized_field(u, v, 0.0f, -0.76f, 0.34f, 0.19f);
		return specification_.size().z * (dome + nose + bridge + chin);
	}

	const FacialSkinShapeSpecification &specification_;
	std::array<FacialApertureDomain, 3> apertures_;
};

class FacialSkinMeshConstruction
{
public:
	explicit FacialSkinMeshConstruction(const FacialSkinShapeSpecification &specification)
		: specification_(specification), surface_(specification), mesh_(std::make_shared<Mesh>())
	{
	}

	GeneratedPrimitiveMesh build()
	{
		addSkinGrid();
		for (const FacialApertureDomain &aperture : surface_.apertures()) addApertureMargin(aperture);
		recomputeNormals();
		mesh_->buildCollisionAccel();
		return GeneratedPrimitiveMesh(mesh_, std::move(surface_tags_));
	}

private:
	int gridOffset(int column, int row) const
	{
		return row * (specification_.horizontalSegments() + 1) + column;
	}

	int addVertex(const glm::vec3 &position, const glm::vec3 &texcoord)
	{
		mesh_->vertices.push_back(position);
		mesh_->texcoords.push_back(texcoord);
		return static_cast<int>(mesh_->vertices.size() - 1);
	}

	void addTriangle(int first, int second, int third, unsigned boundary = 0)
	{
		mesh_->faces.emplace_back(first, second, third);
		surface_tags_.emplace_back(MeshSurfaceRole::Outer, boundary);
	}

	void addSkinGrid()
	{
		const int horizontal = specification_.horizontalSegments();
		const int vertical = specification_.verticalSegments();
		grid_indices_.assign(static_cast<std::size_t>((horizontal + 1) * (vertical + 1)), -1);
		for (int row = 0; row <= vertical; ++row) {
			for (int column = 0; column <= horizontal; ++column) {
				bool inside_aperture = false;
				for (const auto &aperture : surface_.apertures()) {
					inside_aperture |= aperture.containsInteriorVertex(column, row);
				}
				if (inside_aperture) continue;
				const float texture_u = static_cast<float>(column) / horizontal;
				const float texture_v = static_cast<float>(row) / vertical;
				grid_indices_[gridOffset(column, row)] = addVertex(
					surface_.evaluatePosition(2.0f * texture_u - 1.0f, 2.0f * texture_v - 1.0f),
					glm::vec3(texture_u, texture_v, 0.0f));
			}
		}
		for (int row = 0; row < vertical; ++row) {
			for (int column = 0; column < horizontal; ++column) {
				bool inside_aperture = false;
				for (const auto &aperture : surface_.apertures()) {
					inside_aperture |= aperture.containsCell(column, row);
				}
				if (inside_aperture) continue;
				const int first = grid_indices_[gridOffset(column, row)];
				const int second = grid_indices_[gridOffset(column + 1, row)];
				const int third = grid_indices_[gridOffset(column + 1, row + 1)];
				const int fourth = grid_indices_[gridOffset(column, row + 1)];
				addTriangle(first, second, third);
				addTriangle(first, third, fourth);
			}
		}
	}

	void addApertureMargin(const FacialApertureDomain &aperture)
	{
		std::vector<int> outer_margin;
		for (int column = aperture.first_column; column < aperture.last_column; ++column)
			outer_margin.push_back(grid_indices_[gridOffset(column, aperture.first_row)]);
		for (int row = aperture.first_row; row < aperture.last_row; ++row)
			outer_margin.push_back(grid_indices_[gridOffset(aperture.last_column, row)]);
		for (int column = aperture.last_column; column > aperture.first_column; --column)
			outer_margin.push_back(grid_indices_[gridOffset(column, aperture.last_row)]);
		for (int row = aperture.last_row; row > aperture.first_row; --row)
			outer_margin.push_back(grid_indices_[gridOffset(aperture.first_column, row)]);
		std::vector<int> inner_margin;
		const float margin_depth = specification_.size().z *
			(aperture.purpose == FacialAperturePurpose::Mouth ? 0.035f : 0.004f);
		for (int outer_index : outer_margin) {
			glm::vec3 position = mesh_->vertices[outer_index];
			position.z -= margin_depth;
			inner_margin.push_back(addVertex(position, mesh_->texcoords[outer_index]));
		}
		const unsigned boundary = aperture.purpose == FacialAperturePurpose::LeftEye ? 1u :
			aperture.purpose == FacialAperturePurpose::RightEye ? 2u : 3u;
		for (std::size_t index = 0; index < outer_margin.size(); ++index) {
			const std::size_t next = (index + 1) % outer_margin.size();
			addTriangle(outer_margin[index], outer_margin[next], inner_margin[next], boundary);
			addTriangle(outer_margin[index], inner_margin[next], inner_margin[index], boundary);
		}
	}

	void recomputeNormals()
	{
		mesh_->normals.assign(mesh_->vertices.size(), glm::vec3(0.0f));
		mesh_->face_normals.clear();
		mesh_->face_normals.reserve(mesh_->faces.size());
		for (const glm::ivec3 &face : mesh_->faces) {
			const glm::vec3 area_normal = glm::cross(mesh_->vertices[face.y] - mesh_->vertices[face.x],
				mesh_->vertices[face.z] - mesh_->vertices[face.x]);
			const float magnitude = glm::length(area_normal);
			const glm::vec3 face_normal = magnitude > 0.0f ? area_normal / magnitude : glm::vec3(0, 0, 1);
			mesh_->face_normals.push_back(face_normal);
			for (int corner = 0; corner < 3; ++corner) {
				const int index = face[corner];
				const glm::vec3 first = mesh_->vertices[face[(corner + 1) % 3]] - mesh_->vertices[index];
				const glm::vec3 second = mesh_->vertices[face[(corner + 2) % 3]] - mesh_->vertices[index];
				const float angle = std::atan2(glm::length(glm::cross(first, second)), glm::dot(first, second));
				mesh_->normals[index] += angle * face_normal;
			}
		}
		for (glm::vec3 &normal : mesh_->normals) {
			const float magnitude = glm::length(normal);
			normal = magnitude > 0.0f ? normal / magnitude : glm::vec3(0, 0, 1);
		}
	}

	const FacialSkinShapeSpecification &specification_;
	FacialSkinSurfaceEvaluation surface_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<int> grid_indices_;
	std::vector<MeshSurfaceTag> surface_tags_;
};

} // namespace

GeneratedPrimitiveMesh FacialSkinMeshGenerator::generate(
	const ShapeSpecification &specification, std::string *diagnostic) const
{
	const auto *face = dynamic_cast<const FacialSkinShapeSpecification *>(&specification);
	if (face == nullptr) {
		if (diagnostic != nullptr) *diagnostic = "FacialSkinMeshGenerator requires a FacialSkinShapeSpecification.";
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}
	return FacialSkinMeshConstruction(*face).build();
}

FacialSkinLandmarks FacialSkinMeshGenerator::evaluateLandmarks(
	const FacialSkinShapeSpecification &specification) const
{
	FacialSkinSurfaceEvaluation surface(specification);
	FacialSkinLandmarks landmarks;
	landmarks.eyeball_radius = surface.eyeballRadius();
	const auto &left_eye = surface.apertures()[0];
	const auto &right_eye = surface.apertures()[1];
	const auto &mouth = surface.apertures()[2];
	landmarks.left_eyeball_center = surface.eyeballCenter(left_eye);
	landmarks.right_eyeball_center = surface.eyeballCenter(right_eye);
	auto margin = [&surface](const FacialApertureDomain &aperture, float u, float v) {
		return surface.evaluatePosition(aperture.center_u + u * aperture.radius_u,
		                                aperture.center_v + v * aperture.radius_v);
	};
	landmarks.left_eye_outer_corner = margin(left_eye, 1, 0);
	landmarks.left_eye_inner_corner = margin(left_eye, -1, 0);
	landmarks.left_eye_upper_margin = margin(left_eye, 0, 1);
	landmarks.left_eye_lower_margin = margin(left_eye, 0, -1);
	landmarks.right_eye_outer_corner = margin(right_eye, -1, 0);
	landmarks.right_eye_inner_corner = margin(right_eye, 1, 0);
	landmarks.right_eye_upper_margin = margin(right_eye, 0, 1);
	landmarks.right_eye_lower_margin = margin(right_eye, 0, -1);
	landmarks.mouth_left_corner = margin(mouth, 1, 0);
	landmarks.mouth_right_corner = margin(mouth, -1, 0);
	landmarks.mouth_upper_margin = margin(mouth, 0, 1);
	landmarks.mouth_lower_margin = margin(mouth, 0, -1);
	return landmarks;
}
