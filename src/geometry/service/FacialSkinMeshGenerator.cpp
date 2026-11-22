#include "geometry/service/FacialSkinMeshGenerator.h"

#include "geometry/model/FacialSkinShapeSpecification.h"
#include "geometry/model/FacialSkinMesh.h"
#include "geometry/service/FacialElasticModelAssembler.h"
#include "geometry/service/FacialElasticSolver.h"
#include "geometry/service/FacialSkinAdmissibility.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

static FacialSkinLandmarks evaluateAnalyticalLandmarks(const FacialSkinShapeSpecification &specification);

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

// A neutral material-space annulus. Sampling changes discretization only:
// every front point is evaluated by the unchanged facial surface function.
class FacialMouthAnnulusSampling
{
public:
	FacialMouthAnnulusSampling(const FacialSkinShapeSpecification &specification,
		const FacialSkinSurfaceEvaluation &surface)
		: specification_(specification), surface_(surface)
	{
		const int horizontal_scale = specification.horizontalSegments() / 48;
		const int vertical_scale = specification.verticalSegments() / 64;
		first_column_ = 10 * horizontal_scale;
		last_column_ = 38 * horizontal_scale;
		first_row_ = 12 * vertical_scale;
		last_row_ = 28 * vertical_scale;
		middle_column_ = 24 * horizontal_scale;
		middle_row_ = 20 * vertical_scale;
		radial_bands_ = 6 * std::max(horizontal_scale, vertical_scale);
		rim_strips_ = 2 * std::max(horizontal_scale, vertical_scale);
		buildOuterPerimeter();
		buildMaterialRings();
	}

	bool containsInteriorVertex(int column, int row) const
	{
		return column > first_column_ && column < last_column_ &&
			row > first_row_ && row < last_row_;
	}

	bool containsCell(int column, int row) const
	{
		return column >= first_column_ && column < last_column_ &&
			row >= first_row_ && row < last_row_;
	}

	int radialBands() const { return radial_bands_; }
	int rimStrips() const { return rim_strips_; }
	const std::vector<glm::ivec2> &outerGridCoordinates() const { return outer_grid_coordinates_; }
	const std::vector<std::vector<glm::vec2>> &materialRings() const { return material_rings_; }

private:
	static constexpr double pi_ = 3.1415926535897932384626433832795;
	static constexpr int arc_samples_ = 256;

	glm::dvec2 center() const { return glm::dvec2(0.5, 20.0 / 64.0); }
	glm::dvec2 innerHalfExtents() const { return glm::dvec2(10.0 / 48.0, 4.0 / 64.0); }
	glm::dvec2 outerHalfExtents() const { return glm::dvec2(14.0 / 48.0, 8.0 / 64.0); }

	static double rectangleRadius(const glm::dvec2 &direction, const glm::dvec2 &half_extents)
	{
		const double horizontal = std::fabs(direction.x) > 1.0e-15
			? half_extents.x / std::fabs(direction.x) : std::numeric_limits<double>::infinity();
		const double vertical = std::fabs(direction.y) > 1.0e-15
			? half_extents.y / std::fabs(direction.y) : std::numeric_limits<double>::infinity();
		return std::min(horizontal, vertical);
	}

	glm::vec2 boundaryCoordinate(double angle) const
	{
		const glm::dvec2 direction(std::cos(angle), std::sin(angle));
		return glm::vec2(center() + rectangleRadius(direction, innerHalfExtents()) * direction);
	}

	glm::dvec3 surfacePosition(const glm::vec2 &uv) const
	{
		return glm::dvec3(surface_.evaluatePosition(2.0f * uv.x - 1.0f, 2.0f * uv.y - 1.0f));
	}

	static double parameterAtArcFraction(const std::vector<double> &lengths, double fraction)
	{
		if (fraction <= 0.0) return 0.0;
		if (fraction >= 1.0) return 1.0;
		if (!(lengths.back() > 0.0)) throw std::runtime_error("Facial mouth sampling encountered a zero-length arc.");
		const double target = fraction * lengths.back();
		const auto after = std::lower_bound(lengths.begin(), lengths.end(), target);
		const std::size_t index = static_cast<std::size_t>(after - lengths.begin());
		const double interval = lengths[index] - lengths[index - 1];
		const double local = interval > 0.0 ? (target - lengths[index - 1]) / interval : 0.0;
		return (static_cast<double>(index - 1) + local) / static_cast<double>(lengths.size() - 1);
	}

	void buildOuterPerimeter()
	{
		// Counterclockwise from the right cardinal. Each quadrant contains the
		// same number of existing grid edges, including anisotropic detail levels.
		for (int row = middle_row_; row < last_row_; ++row) outer_grid_coordinates_.emplace_back(last_column_, row);
		for (int column = last_column_; column > middle_column_; --column) outer_grid_coordinates_.emplace_back(column, last_row_);
		for (int column = middle_column_; column > first_column_; --column) outer_grid_coordinates_.emplace_back(column, last_row_);
		for (int row = last_row_; row > middle_row_; --row) outer_grid_coordinates_.emplace_back(first_column_, row);
		for (int row = middle_row_; row > first_row_; --row) outer_grid_coordinates_.emplace_back(first_column_, row);
		for (int column = first_column_; column < middle_column_; ++column) outer_grid_coordinates_.emplace_back(column, first_row_);
		for (int column = middle_column_; column < last_column_; ++column) outer_grid_coordinates_.emplace_back(column, first_row_);
		for (int row = first_row_; row < middle_row_; ++row) outer_grid_coordinates_.emplace_back(last_column_, row);
	}

	glm::vec2 radialCoordinate(double inner_angle, double outer_angle,
		const glm::vec2 &inner, const glm::vec2 &outer, double fraction) const
	{
		if (fraction <= 0.0) return inner;
		if (fraction >= 1.0) return outer;
		const double angle = (1.0 - fraction) * inner_angle + fraction * outer_angle;
		const glm::dvec2 direction(std::cos(angle), std::sin(angle));
		// Nested rectangle-intersection radii keep the material path outside the
		// aperture. Straight interpolation between differently distributed loop
		// points can cut through a corner of the hole.
		const double radius = (1.0 - fraction) * rectangleRadius(direction, innerHalfExtents()) +
			fraction * rectangleRadius(direction, outerHalfExtents());
		return glm::vec2(center() + radius * direction);
	}

	void buildMaterialRings()
	{
		const std::size_t count = outer_grid_coordinates_.size();
		std::vector<double> inner_angles(count, 0.0);
		std::vector<glm::vec2> inner_coordinates(count);
		const int horizontal_half = last_column_ - middle_column_;
		const int vertical_half = last_row_ - middle_row_;
		const std::array<int, 8> segment_counts{{vertical_half, horizontal_half, horizontal_half, vertical_half,
			vertical_half, horizontal_half, horizontal_half, vertical_half}};
		const double corner = std::atan2(innerHalfExtents().y, innerHalfExtents().x);
		const std::array<double, 9> angles{{0.0, corner, 0.5 * pi_, pi_ - corner, pi_,
			pi_ + corner, 1.5 * pi_, 2.0 * pi_ - corner, 2.0 * pi_}};
		const std::array<glm::vec2, 8> anchors{{
			{34.0f / 48.0f, 20.0f / 64.0f}, {34.0f / 48.0f, 24.0f / 64.0f},
			{24.0f / 48.0f, 24.0f / 64.0f}, {14.0f / 48.0f, 24.0f / 64.0f},
			{14.0f / 48.0f, 20.0f / 64.0f}, {14.0f / 48.0f, 16.0f / 64.0f},
			{24.0f / 48.0f, 16.0f / 64.0f}, {34.0f / 48.0f, 16.0f / 64.0f}}};
		std::size_t offset = 0;
		for (std::size_t segment = 0; segment < segment_counts.size(); ++segment) {
			const double start = angles[segment];
			const double span = angles[segment + 1] - start;
			std::vector<double> lengths(arc_samples_ + 1, 0.0);
			glm::dvec3 previous = surfacePosition(boundaryCoordinate(start));
			for (int sample = 1; sample <= arc_samples_; ++sample) {
				const glm::dvec3 next = surfacePosition(boundaryCoordinate(start +
					span * static_cast<double>(sample) / arc_samples_));
				lengths[sample] = lengths[sample - 1] + glm::length(next - previous);
				previous = next;
			}
			for (int point = 0; point < segment_counts[segment]; ++point) {
				const std::size_t index = offset + point;
				const double fraction = static_cast<double>(point) / segment_counts[segment];
				inner_angles[index] = start + span * parameterAtArcFraction(lengths, fraction);
				inner_coordinates[index] = boundaryCoordinate(inner_angles[index]);
			}
			// Pin both semantic cardinals and material rectangle corners. Omitting
			// the latter would make a straight UV boundary edge cut into the hole.
			inner_coordinates[offset] = anchors[segment];
			offset += segment_counts[segment];
		}
		material_rings_.assign(radial_bands_ + 1, std::vector<glm::vec2>(count));
		for (std::size_t index = 0; index < count; ++index) {
			const glm::ivec2 grid = outer_grid_coordinates_[index];
			const glm::vec2 outer(static_cast<float>(grid.x) / specification_.horizontalSegments(),
				static_cast<float>(grid.y) / specification_.verticalSegments());
			const glm::dvec2 offset = glm::dvec2(outer) - center();
			double outer_angle = std::atan2(offset.y, offset.x);
			if (outer_angle < 0.0) outer_angle += 2.0 * pi_;
			std::vector<double> lengths(arc_samples_ + 1, 0.0);
			glm::dvec3 previous = surfacePosition(inner_coordinates[index]);
			for (int sample = 1; sample <= arc_samples_; ++sample) {
				const glm::vec2 uv = radialCoordinate(inner_angles[index], outer_angle,
					inner_coordinates[index], outer, static_cast<double>(sample) / arc_samples_);
				const glm::dvec3 next = surfacePosition(uv);
				lengths[sample] = lengths[sample - 1] + glm::length(next - previous);
				previous = next;
			}
			for (int band = 0; band <= radial_bands_; ++band) {
				const double fraction = parameterAtArcFraction(lengths, static_cast<double>(band) / radial_bands_);
				material_rings_[band][index] = radialCoordinate(inner_angles[index], outer_angle,
					inner_coordinates[index], outer, fraction);
			}
		}
	}

	const FacialSkinShapeSpecification &specification_;
	const FacialSkinSurfaceEvaluation &surface_;
	int first_column_ = 0;
	int last_column_ = 0;
	int first_row_ = 0;
	int last_row_ = 0;
	int middle_column_ = 0;
	int middle_row_ = 0;
	int radial_bands_ = 0;
	int rim_strips_ = 0;
	std::vector<glm::ivec2> outer_grid_coordinates_;
	std::vector<std::vector<glm::vec2>> material_rings_;
};

// A single smooth vertex normal must lie in the common hemisphere of its
// incident facets. At a sharp lip cardinal, an angle-weighted average can
// point behind one bank even when a compatible normal exists. This service
// changes shading only; positions, connectivity and mechanical normals stay
// untouched. The finite candidate set finds the centre of the smallest
// spherical cap containing the incident unit normals.
class FacialCompatibleShadingNormalSelection
{
public:
	glm::vec3 select(const glm::vec3 &preferred_normal,
		const std::vector<glm::dvec3> &incident_normals, std::size_t vertex_index) const
	{
		const glm::dvec3 preferred = glm::normalize(glm::dvec3(preferred_normal));
		const double preferred_margin = minimumDot(preferred, incident_normals);
		if (preferred_margin > 1.0e-5) return preferred_normal;
		glm::dvec3 best = preferred;
		double best_margin = preferred_margin;
		const auto consider = [&](const glm::dvec3 &candidate) {
			const double magnitude = glm::length(candidate);
			if (magnitude < 1.0e-12) return;
			const glm::dvec3 direction = candidate / magnitude;
			const double margin = minimumDot(direction, incident_normals);
			if (margin > best_margin + 1.0e-12 ||
				(std::fabs(margin - best_margin) <= 1.0e-12 && glm::dot(direction, preferred) > glm::dot(best, preferred))) {
				best = direction;
				best_margin = margin;
			}
		};
		for (std::size_t first = 0; first < incident_normals.size(); ++first) {
			consider(incident_normals[first]);
			for (std::size_t second = first + 1; second < incident_normals.size(); ++second) {
				consider(incident_normals[first] + incident_normals[second]);
				for (std::size_t third = second + 1; third < incident_normals.size(); ++third) {
					const glm::dvec3 intersection = glm::cross(incident_normals[first] - incident_normals[second],
						incident_normals[first] - incident_normals[third]);
					consider(intersection);
					consider(-intersection);
				}
			}
		}
		if (best_margin <= 1.0e-7)
			throw std::runtime_error("Facial quality mesh vertex " + std::to_string(vertex_index) +
				" has no stable common shading-normal hemisphere.");
		// Keep as much of the usual angle-weighted direction as possible while
		// retaining a positive margin after conversion back to float normals.
		const double required_margin = std::min(1.0e-3, 0.25 * best_margin);
		double fraction = 0.0;
		for (const auto &normal : incident_normals) {
			const double before = glm::dot(preferred, normal);
			if (before >= required_margin) continue;
			fraction = std::max(fraction, (required_margin - before) / (glm::dot(best, normal) - before));
		}
		const glm::dvec3 selected = glm::normalize((1.0 - fraction) * preferred + fraction * best);
		return glm::normalize(glm::vec3(selected));
	}

private:
	static double minimumDot(const glm::dvec3 &direction, const std::vector<glm::dvec3> &normals)
	{
		double minimum = 1.0;
		for (const auto &normal : normals) minimum = std::min(minimum, glm::dot(direction, normal));
		return minimum;
	}
};

class FacialSkinMeshConstruction
{
public:
	explicit FacialSkinMeshConstruction(const FacialSkinShapeSpecification &specification)
		: specification_(specification), surface_(specification), mesh_(std::make_shared<FacialSkinMesh>())
	{
		if (specification.mouthMeshMode() == FacialMouthMeshMode::Quality)
			mouth_annulus_ = std::make_unique<FacialMouthAnnulusSampling>(specification, surface_);
	}

	GeneratedPrimitiveMesh build()
	{
		addSkinGrid();
		if (mouth_annulus_) addQualityMouthAnnulus();
		for (const FacialApertureDomain &aperture : surface_.apertures()) addApertureMargin(aperture);
		mesh_->landmarks = evaluateAnalyticalLandmarks(specification_);
		if (specification_.responseMode() == FacialSkinResponseMode::Elastic) applyElasticResponse();
		recomputeNormals();
		mesh_->buildCollisionAccel();
		return GeneratedPrimitiveMesh(mesh_, std::move(surface_tags_));
	}

private:
	void applyElasticResponse()
	{
		auto state = std::make_shared<FacialSkinMechanicalState>(
			FacialElasticModelAssembler().assemble(specification_, *mesh_, surface_tags_));
		FacialSkinAdmissibility admissibility(*mesh_, surface_tags_, mesh_->landmarks,
			state->system.fixed_vertices, specification_.size().x);
		if (admissibility.minimumGlobeClearance(state->system.rest_positions) < 0.0)
			throw std::runtime_error("Neutral FacialSkin geometry intersects an eye globe at these proportions.");
		FacialElasticSolverSettings settings;
		settings.max_iterations = specification_.elasticSolveSettings().maximum_iterations;
		settings.relative_force_tolerance = specification_.elasticSolveSettings().relative_force_tolerance;
		state->solve_result = FacialElasticSolver().solve(state->system, settings,
			[&admissibility](const auto &positions) { return admissibility.accepts(positions); });
		const auto &result = state->solve_result;
		if (!result.converged) {
			std::ostringstream message;
			message << "FacialSkin elastic equilibrium did not converge: " << result.termination_reason
			        << "; iterations=" << result.iterations
			        << "; relative free force=" << result.relative_free_force;
			if (!admissibility.lastRejection().empty()) message << "; " << admissibility.lastRejection();
			throw std::runtime_error(message.str());
		}
		if (result.final_evaluation.maximum_green_strain > 0.35)
			throw std::runtime_error("FacialSkin elastic result exceeds the prototype Green-strain bound of 0.35; reduce actuation.");
		for (std::size_t index = 0; index < mesh_->vertices.size(); ++index)
			mesh_->vertices[index] = glm::vec3(result.positions[index]);
		const std::vector<glm::dvec3> rendered_positions(mesh_->vertices.begin(), mesh_->vertices.end());
		if (!admissibility.accepts(rendered_positions) || admissibility.minimumGlobeClearance(rendered_positions) < 0.0)
			throw std::runtime_error("FacialSkin float mesh failed post-solve geometry/contact checks.");
		// Landmarks refer to actual solved material vertices. Eye centres remain
		// fixed skull-frame references, and no second solve is needed to export.
		glm::vec3 FacialSkinLandmarks::* points[] = {
			&FacialSkinLandmarks::left_eye_outer_corner, &FacialSkinLandmarks::left_eye_inner_corner,
			&FacialSkinLandmarks::left_eye_upper_margin, &FacialSkinLandmarks::left_eye_lower_margin,
			&FacialSkinLandmarks::right_eye_outer_corner, &FacialSkinLandmarks::right_eye_inner_corner,
			&FacialSkinLandmarks::right_eye_upper_margin, &FacialSkinLandmarks::right_eye_lower_margin,
			&FacialSkinLandmarks::mouth_left_corner, &FacialSkinLandmarks::mouth_right_corner,
			&FacialSkinLandmarks::mouth_upper_margin, &FacialSkinLandmarks::mouth_lower_margin};
		for (auto member : points) {
			auto &landmark = mesh_->landmarks.*member;
			std::size_t closest = 0;
			double minimum = std::numeric_limits<double>::infinity();
			for (std::size_t index = 0; index < state->system.rest_positions.size(); ++index) {
				const double distance = glm::length(state->system.rest_positions[index] - glm::dvec3(landmark));
				if (distance < minimum) { minimum = distance; closest = index; }
			}
			if (minimum > specification_.size().x * 1.0e-5)
				throw std::runtime_error("FacialSkin semantic landmark has no rest material vertex.");
			landmark = mesh_->vertices[closest];
		}
		mesh_->mechanical_state = std::move(state);
	}

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

	double triangleQuality(int first, int second, int third, unsigned boundary) const
	{
		if (boundary == 0) {
			const glm::dvec2 edge_one = glm::dvec2(mesh_->texcoords[second]) - glm::dvec2(mesh_->texcoords[first]);
			const glm::dvec2 edge_two = glm::dvec2(mesh_->texcoords[third]) - glm::dvec2(mesh_->texcoords[first]);
			if (edge_one.x * edge_two.y - edge_one.y * edge_two.x <= 0.0) return -1.0;
		}
		const glm::dvec3 first_edge = glm::dvec3(mesh_->vertices[second]) - glm::dvec3(mesh_->vertices[first]);
		const glm::dvec3 second_edge = glm::dvec3(mesh_->vertices[third]) - glm::dvec3(mesh_->vertices[first]);
		const double denominator = glm::dot(first_edge, first_edge) + glm::dot(second_edge, second_edge) +
			glm::dot(second_edge - first_edge, second_edge - first_edge);
		return denominator > 0.0 ? 2.0 * std::sqrt(3.0) * glm::length(glm::cross(first_edge, second_edge)) / denominator : -1.0;
	}

	void addQualityQuadrilateral(int first, int second, int third, int fourth,
		unsigned boundary, std::size_t parity)
	{
		const double first_diagonal = std::min(triangleQuality(first, second, third, boundary),
			triangleQuality(first, third, fourth, boundary));
		const double second_diagonal = std::min(triangleQuality(first, second, fourth, boundary),
			triangleQuality(second, third, fourth, boundary));
		if (std::max(first_diagonal, second_diagonal) <= 0.0)
			throw std::runtime_error("Facial quality mouth mesh has an invalid material quadrilateral.");
		const bool choose_first = std::fabs(first_diagonal - second_diagonal) <= 1.0e-8
			? parity % 2 == 0 : first_diagonal > second_diagonal;
		if (choose_first) {
			addTriangle(first, second, third, boundary);
			addTriangle(first, third, fourth, boundary);
		} else {
			addTriangle(first, second, fourth, boundary);
			addTriangle(second, third, fourth, boundary);
		}
	}

	void addQualityMouthAnnulus()
	{
		const auto &material_rings = mouth_annulus_->materialRings();
		const std::size_t count = mouth_annulus_->outerGridCoordinates().size();
		std::vector<std::vector<int>> rings(material_rings.size(), std::vector<int>(count));
		for (std::size_t band = 0; band < material_rings.size(); ++band) {
			for (std::size_t index = 0; index < count; ++index) {
				if (band + 1 == material_rings.size()) {
					const glm::ivec2 coordinate = mouth_annulus_->outerGridCoordinates()[index];
					rings[band][index] = grid_indices_[gridOffset(coordinate.x, coordinate.y)];
				} else {
					const glm::vec2 uv = material_rings[band][index];
					rings[band][index] = addVertex(surface_.evaluatePosition(2.0f * uv.x - 1.0f, 2.0f * uv.y - 1.0f),
						glm::vec3(uv, 0.0f));
				}
			}
		}
		for (std::size_t band = 0; band + 1 < rings.size(); ++band) {
			for (std::size_t index = 0; index < count; ++index) {
				const std::size_t next = (index + 1) % count;
				addQualityQuadrilateral(rings[band][index], rings[band + 1][index],
					rings[band + 1][next], rings[band][next], 0, band + index);
			}
		}
		quality_mouth_margin_ = std::move(rings.front());
	}

	void addQualityMouthMargin()
	{
		std::vector<int> previous = quality_mouth_margin_;
		const float depth = specification_.size().z * 0.035f;
		const int strips = mouth_annulus_->rimStrips();
		for (int strip = 1; strip <= strips; ++strip) {
			std::vector<int> next;
			for (int outer_index : quality_mouth_margin_) {
				glm::vec3 position = mesh_->vertices[outer_index];
				position.z -= depth * static_cast<float>(strip) / strips;
				next.push_back(addVertex(position, mesh_->texcoords[outer_index]));
			}
			for (std::size_t index = 0; index < previous.size(); ++index) {
				const std::size_t following = (index + 1) % previous.size();
				addQualityQuadrilateral(previous[index], previous[following], next[following], next[index],
					3, index + static_cast<std::size_t>(strip));
			}
			previous = std::move(next);
		}
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
				if (mouth_annulus_) inside_aperture |= mouth_annulus_->containsInteriorVertex(column, row);
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
				if (mouth_annulus_) inside_aperture |= mouth_annulus_->containsCell(column, row);
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
		if (mouth_annulus_ && aperture.purpose == FacialAperturePurpose::Mouth) {
			addQualityMouthMargin();
			return;
		}
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
		if (mouth_annulus_) enforceCompatibleQualityNormals();
	}

	void enforceCompatibleQualityNormals()
	{
		std::vector<std::vector<glm::dvec3>> incident_normals(mesh_->vertices.size());
		for (const auto &face : mesh_->faces) {
			const glm::dvec3 normal = glm::normalize(glm::cross(
				glm::dvec3(mesh_->vertices[face.y]) - glm::dvec3(mesh_->vertices[face.x]),
				glm::dvec3(mesh_->vertices[face.z]) - glm::dvec3(mesh_->vertices[face.x])));
			for (int vertex : {face.x, face.y, face.z}) incident_normals[vertex].push_back(normal);
		}
		const FacialCompatibleShadingNormalSelection selection;
		for (std::size_t index = 0; index < mesh_->normals.size(); ++index)
			mesh_->normals[index] = selection.select(mesh_->normals[index], incident_normals[index], index);
	}

	const FacialSkinShapeSpecification &specification_;
	FacialSkinSurfaceEvaluation surface_;
	std::shared_ptr<FacialSkinMesh> mesh_;
	std::unique_ptr<FacialMouthAnnulusSampling> mouth_annulus_;
	std::vector<int> quality_mouth_margin_;
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
	try {
		return FacialSkinMeshConstruction(*face).build();
	} catch (const std::exception &error) {
		if (diagnostic != nullptr) *diagnostic = error.what();
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}
}

static FacialSkinLandmarks evaluateAnalyticalLandmarks(
	const FacialSkinShapeSpecification &specification)
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

FacialSkinLandmarks FacialSkinMeshGenerator::evaluateLandmarks(
	const FacialSkinShapeSpecification &specification) const
{
	if (specification.responseMode() == FacialSkinResponseMode::Analytic)
		return evaluateAnalyticalLandmarks(specification);
	std::string diagnostic;
	const auto generated = generate(specification, &diagnostic);
	const auto *face_mesh = dynamic_cast<const FacialSkinMesh *>(generated.mesh().get());
	if (face_mesh == nullptr) throw std::runtime_error(diagnostic);
	return face_mesh->landmarks;
}
