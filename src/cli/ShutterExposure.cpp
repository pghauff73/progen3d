#include "ShutterExposure.h"

#include "PreviewColor.h"

#include <stdexcept>

ShutterExposure::ShutterExposure(int width, int height)
	: width_(width), height_(height), linear_sum_(static_cast<std::size_t>(width) * height, glm::vec3(0.0f))
{
}

void ShutterExposure::addSample(const PreviewImage &image)
{
	if (!image.isValid() || image.width() != width_ || image.height() != height_) {
		throw std::runtime_error("A shutter sample has the wrong image dimensions.");
	}
	for (std::size_t pixel = 0; pixel < linear_sum_.size(); ++pixel) {
		linear_sum_[pixel] += preview_color::decode_pixel(image.rgbPixels(), pixel);
	}
	++samples_;
}

PreviewImage ShutterExposure::finish() const
{
	if (samples_ == 0) {
		throw std::logic_error("A shutter exposure needs at least one sample.");
	}
	std::vector<std::uint8_t> pixels(linear_sum_.size() * 3);
	const float weight = 1.0f / static_cast<float>(samples_);
	for (std::size_t pixel = 0; pixel < linear_sum_.size(); ++pixel) {
		preview_color::encode_pixel(linear_sum_[pixel] * weight, pixel, pixels);
	}
	return PreviewImage(width_, height_, std::move(pixels));
}
