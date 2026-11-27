#pragma once

#include "editor/model/PreviewImage.h"

#include <glm/glm.hpp>

class ShutterExposure
{
public:
	ShutterExposure(int width, int height);

	void addSample(const PreviewImage &image);
	PreviewImage finish() const;

private:
	int width_;
	int height_;
	std::vector<glm::vec3> linear_sum_;
	std::uint32_t samples_ = 0;
};
