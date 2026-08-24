#pragma once

#include "vehicle/model/VehicleFittingEvidence.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

enum class VehicleCameraProjection
{
	Orthographic,
	Perspective
};

enum class VehicleConstraintStrength
{
	Hard,
	Soft
};

class VehicleCameraModel
{
public:
	VehicleCameraModel(
		std::string identifier,
		VehicleReferenceView view,
		VehicleCameraProjection projection,
		glm::mat4 world_to_camera,
		glm::vec2 principal_point,
		float focal_length,
		float orthographic_scale,
		glm::vec2 image_dimensions)
		: identifier_(std::move(identifier)),
		  view_(view),
		  projection_(projection),
		  world_to_camera_(world_to_camera),
		  principal_point_(principal_point),
		  focal_length_(focal_length),
		  orthographic_scale_(orthographic_scale),
		  image_dimensions_(image_dimensions)
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleReferenceView view() const { return view_; }
	VehicleCameraProjection projection() const { return projection_; }
	const glm::mat4 &worldToCamera() const { return world_to_camera_; }
	const glm::vec2 &principalPoint() const { return principal_point_; }
	float focalLength() const { return focal_length_; }
	float orthographicScale() const { return orthographic_scale_; }
	const glm::vec2 &imageDimensions() const { return image_dimensions_; }

private:
	std::string identifier_;
	VehicleReferenceView view_ = VehicleReferenceView::Front;
	VehicleCameraProjection projection_ = VehicleCameraProjection::Orthographic;
	glm::mat4 world_to_camera_{1.0f};
	glm::vec2 principal_point_{0.5f};
	float focal_length_ = 1.0f;
	float orthographic_scale_ = 1.0f;
	glm::vec2 image_dimensions_{1.0f};
};

class VehicleLandmarkObservation
{
public:
	VehicleLandmarkObservation(
		std::string camera_identifier,
		glm::vec2 image_point,
		float uncertainty,
		float confidence)
		: camera_identifier_(std::move(camera_identifier)),
		  image_point_(image_point),
		  uncertainty_(uncertainty),
		  confidence_(confidence)
	{
	}

	const std::string &cameraIdentifier() const { return camera_identifier_; }
	const glm::vec2 &imagePoint() const { return image_point_; }
	float uncertainty() const { return uncertainty_; }
	float confidence() const { return confidence_; }

private:
	std::string camera_identifier_;
	glm::vec2 image_point_{0.0f};
	float uncertainty_ = 0.0f;
	float confidence_ = 0.0f;
};

class VehicleLandmark
{
public:
	VehicleLandmark(
		std::string identifier,
		glm::vec3 position,
		std::vector<VehicleLandmarkObservation> observations,
		std::string symmetry_partner_identifier,
		std::string topology_role,
		VehicleEvidenceClassification evidence_classification,
		VehicleConstraintStrength constraint_strength)
		: identifier_(std::move(identifier)),
		  position_(position),
		  observations_(std::move(observations)),
		  symmetry_partner_identifier_(std::move(symmetry_partner_identifier)),
		  topology_role_(std::move(topology_role)),
		  evidence_classification_(evidence_classification),
		  constraint_strength_(constraint_strength)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &position() const { return position_; }
	const std::vector<VehicleLandmarkObservation> &observations() const
	{
		return observations_;
	}
	const std::string &symmetryPartnerIdentifier() const
	{
		return symmetry_partner_identifier_;
	}
	const std::string &topologyRole() const { return topology_role_; }
	VehicleEvidenceClassification evidenceClassification() const
	{
		return evidence_classification_;
	}
	VehicleConstraintStrength constraintStrength() const
	{
		return constraint_strength_;
	}

private:
	std::string identifier_;
	glm::vec3 position_{0.0f};
	std::vector<VehicleLandmarkObservation> observations_;
	std::string symmetry_partner_identifier_;
	std::string topology_role_;
	VehicleEvidenceClassification evidence_classification_ =
		VehicleEvidenceClassification::Observed;
	VehicleConstraintStrength constraint_strength_ = VehicleConstraintStrength::Soft;
};

class CharacterLineObservation
{
public:
	CharacterLineObservation(
		std::string identifier,
		std::string camera_identifier,
		std::vector<glm::vec2> image_points,
		float uncertainty,
		float confidence)
		: identifier_(std::move(identifier)),
		  camera_identifier_(std::move(camera_identifier)),
		  image_points_(std::move(image_points)),
		  uncertainty_(uncertainty),
		  confidence_(confidence)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &cameraIdentifier() const { return camera_identifier_; }
	const std::vector<glm::vec2> &imagePoints() const { return image_points_; }
	float uncertainty() const { return uncertainty_; }
	float confidence() const { return confidence_; }

private:
	std::string identifier_;
	std::string camera_identifier_;
	std::vector<glm::vec2> image_points_;
	float uncertainty_ = 0.0f;
	float confidence_ = 0.0f;
};

class VehicleViewObservation
{
public:
	VehicleViewObservation(
		std::string identifier,
		VehicleCameraModel camera,
		std::string image_identifier,
		std::vector<glm::vec2> silhouette,
		std::vector<std::string> landmark_identifiers,
		std::vector<CharacterLineObservation> character_lines,
		std::vector<CharacterLineObservation> panel_lines,
		std::vector<CharacterLineObservation> glazing_lines,
		std::vector<CharacterLineObservation> wheel_ellipses)
		: identifier_(std::move(identifier)),
		  camera_(std::move(camera)),
		  image_identifier_(std::move(image_identifier)),
		  silhouette_(std::move(silhouette)),
		  landmark_identifiers_(std::move(landmark_identifiers)),
		  character_lines_(std::move(character_lines)),
		  panel_lines_(std::move(panel_lines)),
		  glazing_lines_(std::move(glazing_lines)),
		  wheel_ellipses_(std::move(wheel_ellipses))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const VehicleCameraModel &camera() const { return camera_; }
	const std::string &imageIdentifier() const { return image_identifier_; }
	const std::vector<glm::vec2> &silhouette() const { return silhouette_; }
	const std::vector<std::string> &landmarkIdentifiers() const
	{
		return landmark_identifiers_;
	}
	const std::vector<CharacterLineObservation> &characterLines() const
	{
		return character_lines_;
	}
	const std::vector<CharacterLineObservation> &panelLines() const
	{
		return panel_lines_;
	}
	const std::vector<CharacterLineObservation> &glazingLines() const
	{
		return glazing_lines_;
	}
	const std::vector<CharacterLineObservation> &wheelEllipses() const
	{
		return wheel_ellipses_;
	}

private:
	std::string identifier_;
	VehicleCameraModel camera_;
	std::string image_identifier_;
	std::vector<glm::vec2> silhouette_;
	std::vector<std::string> landmark_identifiers_;
	std::vector<CharacterLineObservation> character_lines_;
	std::vector<CharacterLineObservation> panel_lines_;
	std::vector<CharacterLineObservation> glazing_lines_;
	std::vector<CharacterLineObservation> wheel_ellipses_;
};

class VehicleObservationSet
{
public:
	explicit VehicleObservationSet(std::vector<VehicleViewObservation> observations)
		: observations_(std::move(observations))
	{
	}

	const std::vector<VehicleViewObservation> &observations() const
	{
		return observations_;
	}
	const VehicleViewObservation *findByCamera(const std::string &identifier) const
	{
		for (const VehicleViewObservation &observation : observations_) {
			if (observation.camera().identifier() == identifier) return &observation;
		}
		return nullptr;
	}

private:
	std::vector<VehicleViewObservation> observations_;
};

class SilhouetteConstraint
{
public:
	SilhouetteConstraint(
		std::string identifier,
		std::string camera_identifier,
		std::vector<glm::vec3> model_boundary_points,
		std::vector<glm::vec2> observed_silhouette,
		float weight,
		VehicleConstraintStrength strength,
		float tolerance)
		: identifier_(std::move(identifier)),
		  camera_identifier_(std::move(camera_identifier)),
		  model_boundary_points_(std::move(model_boundary_points)),
		  observed_silhouette_(std::move(observed_silhouette)),
		  weight_(weight),
		  strength_(strength),
		  tolerance_(tolerance)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &cameraIdentifier() const { return camera_identifier_; }
	const std::vector<glm::vec3> &modelBoundaryPoints() const
	{
		return model_boundary_points_;
	}
	const std::vector<glm::vec2> &observedSilhouette() const
	{
		return observed_silhouette_;
	}
	float weight() const { return weight_; }
	VehicleConstraintStrength strength() const { return strength_; }
	float tolerance() const { return tolerance_; }

private:
	std::string identifier_;
	std::string camera_identifier_;
	std::vector<glm::vec3> model_boundary_points_;
	std::vector<glm::vec2> observed_silhouette_;
	float weight_ = 0.0f;
	VehicleConstraintStrength strength_ = VehicleConstraintStrength::Soft;
	float tolerance_ = 0.0f;
};
