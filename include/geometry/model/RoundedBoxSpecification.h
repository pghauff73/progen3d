#pragma once

class RoundedBoxSpecification
{
public:
	RoundedBoxSpecification(
		float width,
		float height,
		float depth,
		float corner_radius,
		int segments_per_corner = 3)
		: width_(width),
		  height_(height),
		  depth_(depth),
		  corner_radius_(corner_radius),
		  segments_per_corner_(segments_per_corner)
	{
	}

	float width() const { return width_; }
	float height() const { return height_; }
	float depth() const { return depth_; }
	float cornerRadius() const { return corner_radius_; }
	int segmentsPerCorner() const { return segments_per_corner_; }

private:
	float width_ = 0.0f;
	float height_ = 0.0f;
	float depth_ = 0.0f;
	float corner_radius_ = 0.0f;
	int segments_per_corner_ = 0;
};
