#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class BinarySilhouette
{
public:
	BinarySilhouette() = default;
	BinarySilhouette(std::size_t width, std::size_t height)
		: width_(width), height_(height), pixels_(width * height, 0u)
	{
	}

	std::size_t width() const { return width_; }
	std::size_t height() const { return height_; }

	bool contains(std::size_t x, std::size_t y) const
	{
		return x < width_ && y < height_;
	}

	bool isOccupied(std::size_t x, std::size_t y) const
	{
		return contains(x, y) && pixels_[y * width_ + x] != 0u;
	}

	void occupy(std::size_t x, std::size_t y)
	{
		if (contains(x, y)) pixels_[y * width_ + x] = 1u;
	}

	std::size_t occupiedPixelCount() const;
	std::uint64_t deterministicHash() const;
	const std::vector<std::uint8_t> &pixels() const { return pixels_; }

private:
	std::size_t width_ = 0u;
	std::size_t height_ = 0u;
	std::vector<std::uint8_t> pixels_;
};
