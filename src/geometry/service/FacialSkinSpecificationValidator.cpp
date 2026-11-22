#include "geometry/service/FacialSkinSpecificationValidator.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

std::shared_ptr<const FacialSkinShapeSpecification>
FacialSkinSpecificationValidator::validate(
	const FacialSkinShapeSpecificationCandidate &candidate,
	std::string *diagnostic) const
{
	for (int axis = 0; axis < 3; ++axis) {
		if (!std::isfinite(candidate.size[axis]) || candidate.size[axis] <= 1.0e-5f ||
		    candidate.size[axis] > 1000.0f) {
			if (diagnostic != nullptr) {
				*diagnostic = "FacialSkin size must contain finite positive dimensions in (0.00001, 1000].";
			}
			return {};
		}
	}
	const float height_ratio = candidate.size.y / candidate.size.x;
	const float depth_ratio = candidate.size.z / candidate.size.x;
	if (height_ratio < 0.5f || height_ratio > 2.0f || depth_ratio < 0.05f || depth_ratio > 1.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "FacialSkin requires height/width in [0.5,2] and depth/width in [0.05,1]; use uniform scale for miniature or large heads.";
		}
		return {};
	}
	// These multiples keep all three aperture boundaries and semantic landmarks
	// at exactly the same rest coordinates at every supported detail level.
	if (candidate.horizontal_segments < 48 || candidate.horizontal_segments > 192 ||
	    candidate.horizontal_segments % 48 != 0 ||
	    candidate.vertical_segments < 64 || candidate.vertical_segments > 256 ||
	    candidate.vertical_segments % 64 != 0) {
		if (diagnostic != nullptr) {
			*diagnostic = "FacialSkin segments require horizontal multiples of 48 in [48,192] and vertical multiples of 64 in [64,256].";
		}
		return {};
	}
	const auto &controls = candidate.controls;
	const float values[] = {controls.smile_left, controls.smile_right,
		controls.jaw_open, controls.blink_left, controls.blink_right,
		controls.brow_raise_left, controls.brow_raise_right};
	for (float value : values) {
		if (!std::isfinite(value) || value < 0.0f || value > 1.0f) {
			if (diagnostic != nullptr) {
				*diagnostic = "FacialSkin smile, jawOpen, blink and browRaise controls must be finite values in [0,1].";
			}
			return {};
		}
	}
	std::ostringstream key;
	key << "FacialSkin:v1:" << std::hexfloat
	    << candidate.size.x << ',' << candidate.size.y << ',' << candidate.size.z
	    << ':' << candidate.horizontal_segments << ',' << candidate.vertical_segments;
	for (float value : values) key << ':' << value;
	std::ostringstream canonical;
	canonical << std::setprecision(std::numeric_limits<float>::max_digits10)
	          << "FacialSkin(size(" << candidate.size.x << ' ' << candidate.size.y
	          << ' ' << candidate.size.z << ") segments("
	          << candidate.horizontal_segments << ' ' << candidate.vertical_segments
	          << ") smile(" << controls.smile_left << ' ' << controls.smile_right
	          << ") jawOpen(" << controls.jaw_open << ") blink("
	          << controls.blink_left << ' ' << controls.blink_right
	          << ") browRaise(" << controls.brow_raise_left << ' '
	          << controls.brow_raise_right << "))";
	return std::make_shared<const FacialSkinShapeSpecification>(
		candidate.size, candidate.horizontal_segments, candidate.vertical_segments,
		controls, ShapeSpecificationKey(key.str()), canonical.str());
}
