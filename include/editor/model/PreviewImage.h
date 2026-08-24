#pragma once

#include <cstdint>
#include <utility>
#include <vector>

class PreviewImage
{
public:
	PreviewImage() = default;

	PreviewImage(int width, int height, std::vector<std::uint8_t> rgb_pixels)
		: width_(width),
		  height_(height),
		  rgb_pixels_(std::move(rgb_pixels))
	{
	}

	bool isValid() const
	{
		return width_ > 0 && height_ > 0 &&
		       rgb_pixels_.size() ==
		           static_cast<std::size_t>(width_) *
		               static_cast<std::size_t>(height_) * 3u;
	}

	int width() const { return width_; }
	int height() const { return height_; }
	const std::vector<std::uint8_t> &rgbPixels() const { return rgb_pixels_; }

private:
	int width_ = 0;
	int height_ = 0;
	std::vector<std::uint8_t> rgb_pixels_;
};
