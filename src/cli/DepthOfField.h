#pragma once

#include "ShotCamera.h"
#include "editor/model/PreviewImage.h"

#include <vector>

class RenderRowExecutor;

struct DepthOfFieldResult {
	PreviewImage color;
	std::vector<std::uint8_t> coverage;
};

class DepthOfField
{
public:
	DepthOfField(const ShotCameraSample &shot, int image_height, RenderRowExecutor &rows);

	bool enabled() const;
	float foregroundCutoff() const;
	std::vector<std::uint8_t> foregroundCoverage(int width, int height,
	                                             const std::vector<float> &sharp_depth) const;
	DepthOfFieldResult composite(const PreviewImage &sharp, const std::vector<float> &sharp_depth,
	                             const PreviewImage &far_plate, const std::vector<float> &far_depth) const;

private:
	float blurRadius(float depth) const;
	int foregroundRadius(float depth, float cutoff) const;
	RenderRowExecutor &rows_;
	double blur_scale_ = 0.0;
	double inverse_focus_depth_ = 0.0;
	float max_blur_px_ = 0.0f;
};
