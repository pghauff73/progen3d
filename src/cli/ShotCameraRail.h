#pragma once

#include "ShotCamera.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

using ShotRailCurve = std::array<glm::dvec3, 4>;

struct ShotRailPose {
	glm::dvec3 position;
	glm::dvec3 up;
	ShotRailSample motion;
};

class ShotCameraRail
{
public:
	ShotCameraRail(std::string id, std::vector<ShotRailCurve> curves,
	              std::vector<ShotKeyframe<double>> progress,
	              std::optional<ShotAttachment> anchor, glm::dvec3 up);

	ShotRailPose sample(const SceneGenerationContext &scene, double time_seconds) const;

private:
	struct ArcSample {
		double parameter;
		double distance;
		glm::dvec3 tangent;
		glm::dvec3 up;
	};

	struct Segment {
		ShotRailCurve curve;
		std::vector<ArcSample> samples;
	};

	std::pair<double, double> sampleProgress(double time_seconds) const;

	std::string id_;
	std::vector<Segment> segments_;
	std::vector<ShotKeyframe<double>> progress_;
	std::vector<double> progress_slopes_;
	std::optional<ShotAttachment> anchor_;
	double length_ = 0.0;
};
