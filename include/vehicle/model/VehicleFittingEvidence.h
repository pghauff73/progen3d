#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

enum class VehicleReferenceView
{
	Front,
	Rear,
	Left,
	Right,
	Top
};

enum class VehicleEvidenceClassification
{
	Observed,
	Triangulated,
	EngineeringInference
};

class ViewSilhouetteSample
{
public:
	ViewSilhouetteSample(std::string semantic_name, glm::vec2 image_point)
		: semantic_name_(std::move(semantic_name)), image_point_(image_point)
	{
	}

	const std::string &semanticName() const { return semantic_name_; }
	const glm::vec2 &imagePoint() const { return image_point_; }

private:
	std::string semantic_name_;
	glm::vec2 image_point_{0.0f};
};

class VehicleViewEnvelope
{
public:
	VehicleViewEnvelope(
		std::string identifier,
		VehicleReferenceView view,
		std::string camera_identifier,
		std::vector<ViewSilhouetteSample> silhouette_samples,
		float fitting_weight)
		: identifier_(std::move(identifier)),
		  view_(view),
		  camera_identifier_(std::move(camera_identifier)),
		  silhouette_samples_(std::move(silhouette_samples)),
		  fitting_weight_(fitting_weight)
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleReferenceView view() const { return view_; }
	const std::string &cameraIdentifier() const { return camera_identifier_; }
	const std::vector<ViewSilhouetteSample> &silhouetteSamples() const
	{
		return silhouette_samples_;
	}
	float fittingWeight() const { return fitting_weight_; }

private:
	std::string identifier_;
	VehicleReferenceView view_ = VehicleReferenceView::Left;
	std::string camera_identifier_;
	std::vector<ViewSilhouetteSample> silhouette_samples_;
	float fitting_weight_ = 0.0f;
};

class MultiViewEnvelope
{
public:
	MultiViewEnvelope(
		std::string identifier,
		std::vector<VehicleViewEnvelope> view_envelopes,
		float projection_tolerance)
		: identifier_(std::move(identifier)),
		  view_envelopes_(std::move(view_envelopes)),
		  projection_tolerance_(projection_tolerance)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<VehicleViewEnvelope> &viewEnvelopes() const
	{
		return view_envelopes_;
	}
	float projectionTolerance() const { return projection_tolerance_; }

private:
	std::string identifier_;
	std::vector<VehicleViewEnvelope> view_envelopes_;
	float projection_tolerance_ = 0.0f;
};

class SurfaceLandmarkObservation
{
public:
	SurfaceLandmarkObservation(
		VehicleReferenceView view,
		glm::vec2 image_point,
		float weight)
		: view_(view), image_point_(image_point), weight_(weight)
	{
	}

	VehicleReferenceView view() const { return view_; }
	const glm::vec2 &imagePoint() const { return image_point_; }
	float weight() const { return weight_; }

private:
	VehicleReferenceView view_ = VehicleReferenceView::Front;
	glm::vec2 image_point_{0.0f};
	float weight_ = 0.0f;
};

class SurfaceLandmark
{
public:
	SurfaceLandmark(
		std::string identifier,
		glm::vec3 fitted_point,
		std::vector<SurfaceLandmarkObservation> observations,
		float confidence,
		VehicleEvidenceClassification evidence_classification)
		: identifier_(std::move(identifier)),
		  fitted_point_(fitted_point),
		  observations_(std::move(observations)),
		  confidence_(confidence),
		  evidence_classification_(evidence_classification)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &fittedPoint() const { return fitted_point_; }
	const std::vector<SurfaceLandmarkObservation> &observations() const
	{
		return observations_;
	}
	float confidence() const { return confidence_; }
	VehicleEvidenceClassification evidenceClassification() const
	{
		return evidence_classification_;
	}

private:
	std::string identifier_;
	glm::vec3 fitted_point_{0.0f};
	std::vector<SurfaceLandmarkObservation> observations_;
	float confidence_ = 0.0f;
	VehicleEvidenceClassification evidence_classification_ =
		VehicleEvidenceClassification::Observed;
};

class VehicleViewEvidenceWeights
{
public:
	VehicleViewEvidenceWeights(
		float front,
		float rear,
		float left,
		float right,
		float top)
		: front_(front), rear_(rear), left_(left), right_(right), top_(top)
	{
	}

	float front() const { return front_; }
	float rear() const { return rear_; }
	float left() const { return left_; }
	float right() const { return right_; }
	float top() const { return top_; }

private:
	float front_ = 0.0f;
	float rear_ = 0.0f;
	float left_ = 0.0f;
	float right_ = 0.0f;
	float top_ = 0.0f;
};
