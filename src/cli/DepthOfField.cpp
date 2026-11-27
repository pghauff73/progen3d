#include "DepthOfField.h"

#include "PreviewColor.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

constexpr float minimum_blur_radius = 0.75f;

struct DiskKernel {
	std::vector<int> half_widths;
	float weight = 1.0f;
};

std::vector<DiskKernel> disk_kernels(int max_radius)
{
	std::vector<DiskKernel> kernels(static_cast<std::size_t>(max_radius) + 1);
	for (int radius = 1; radius <= max_radius; ++radius) {
		DiskKernel &kernel = kernels[radius];
		int pixel_count = 0;
		for (int y = -radius; y <= radius; ++y) {
			const int half_width = static_cast<int>(std::sqrt(radius * radius - y * y));
			kernel.half_widths.push_back(half_width);
			pixel_count += half_width * 2 + 1;
		}
		kernel.weight = 1.0f / static_cast<float>(pixel_count);
	}
	return kernels;
}

// Add a disk as horizontal intervals of premultiplied colour and coverage.
std::vector<glm::vec4> scatter_disks(const PreviewImage &image,
                                         const std::vector<std::uint8_t> &radii, int max_radius)
{
	const int width = image.width();
	const int height = image.height();
	const std::size_t stride = static_cast<std::size_t>(width) + 1;
	std::vector<glm::vec4> sums(stride * height, glm::vec4(0.0f));
	const auto kernels = disk_kernels(max_radius);
	for (int y = -max_radius; y < height + max_radius; ++y) {
		for (int x = -max_radius; x < width + max_radius; ++x) {
			// Extend the image edges so a foreground object can fill the picture edge.
			const std::size_t source = static_cast<std::size_t>(std::clamp(y, 0, height - 1)) * width +
			                           std::clamp(x, 0, width - 1);
			const int radius = radii[source];
			if (radius == 0) {
				continue;
			}
			const DiskKernel &kernel = kernels[radius];
			const glm::vec4 contribution = glm::vec4(preview_color::decode_pixel(image.rgbPixels(), source), 1.0f) *
			                               kernel.weight;
			for (int row = std::max(0, y - radius); row <= std::min(height - 1, y + radius); ++row) {
				const int half_width = kernel.half_widths[row - y + radius];
				const int left = std::max(0, x - half_width);
				const int right = std::min(width - 1, x + half_width);
				if (left <= right) {
					const std::size_t offset = static_cast<std::size_t>(row) * stride;
					sums[offset + left] += contribution;
					sums[offset + right + 1] -= contribution;
				}
			}
		}
	}
	return sums;
}

}

DepthOfField::DepthOfField(const ShotCameraSample &shot, int image_height)
	: blur_scale_(static_cast<double>(image_height) * shot.lens.aperture_radius /
	              (2.0 * std::tan(glm::radians(static_cast<double>(shot.lens.fov_degrees)) * 0.5))),
	  inverse_focus_depth_(1.0 / shot.focus_depth),
	  max_blur_px_(shot.lens.max_blur_px)
{
}

bool DepthOfField::enabled() const
{
	return blur_scale_ > 0.0 && max_blur_px_ >= minimum_blur_radius;
}

float DepthOfField::foregroundCutoff() const
{
	if (!enabled()) {
		return 0.0f;
	}
	return static_cast<float>(1.0 / (inverse_focus_depth_ + minimum_blur_radius / blur_scale_));
}

float DepthOfField::blurRadius(float depth) const
{
	// Zero depth labels sky, modelled at infinity for the lens.
	const double reciprocal_depth = depth > 0.0f ? 1.0 / depth : 0.0;
	return static_cast<float>(std::min(static_cast<double>(max_blur_px_),
	                                  std::abs(blur_scale_ * (inverse_focus_depth_ - reciprocal_depth))));
}

DepthOfFieldResult DepthOfField::composite(const PreviewImage &sharp, const std::vector<float> &sharp_depth,
                                          const PreviewImage &far_plate, const std::vector<float> &far_depth) const
{
	const int width = sharp.width();
	const int height = sharp.height();
	const std::size_t pixel_count = sharp_depth.size();
	const float cutoff = foregroundCutoff();
	std::vector<std::uint8_t> near_radii(pixel_count, 0);
	std::vector<std::uint8_t> far_radii(pixel_count, 0);
	int max_near_radius = 0;
	int max_far_radius = 0;
	for (std::size_t index = 0; index < pixel_count; ++index) {
		if (sharp_depth[index] > 0.0f && sharp_depth[index] < cutoff) {
			const int radius = static_cast<int>(std::lround(blurRadius(sharp_depth[index])));
			near_radii[index] = static_cast<std::uint8_t>(radius);
			max_near_radius = std::max(max_near_radius, radius);
		}
		if (far_depth[index] == 0.0f || far_depth[index] > 1.0 / inverse_focus_depth_) {
			const float blur_radius = blurRadius(far_depth[index]);
			if (blur_radius >= minimum_blur_radius) {
				const int radius = static_cast<int>(std::lround(blur_radius));
				far_radii[index] = static_cast<std::uint8_t>(radius);
				max_far_radius = std::max(max_far_radius, radius);
			}
		}
	}
	std::vector<glm::vec4> near_sums;
	if (max_near_radius > 0) {
		near_sums = scatter_disks(sharp, near_radii, max_near_radius);
	}
	std::vector<glm::vec4> far_sums;
	if (max_far_radius > 0) {
		far_sums = scatter_disks(far_plate, far_radii, max_far_radius);
	}

	std::vector<std::uint8_t> color = far_plate.rgbPixels();
	std::vector<std::uint8_t> coverage(pixel_count, 0);
	for (int y = 0; y < height; ++y) {
		glm::vec4 foreground(0.0f);
		glm::vec4 background(0.0f);
		for (int x = 0; x < width; ++x) {
			const std::size_t index = static_cast<std::size_t>(y) * width + x;
			const std::size_t interval_index = index + static_cast<std::size_t>(y);
			if (!near_sums.empty()) {
				foreground += near_sums[interval_index];
			}
			if (!far_sums.empty()) {
				background += far_sums[interval_index];
			}
			const float alpha = std::clamp(foreground.a, 0.0f, 1.0f);
			if (alpha < 1.0e-5f && far_radii[index] == 0) {
				continue;
			}
			// The far plate excludes defocused foreground. Normalizing its disk
			// weights fills around sharp features without sampling their colours.
			const glm::vec3 far_color = far_radii[index] > 0 && background.a > 1.0e-5f
				? glm::vec3(background) / background.a
				: preview_color::decode_pixel(far_plate.rgbPixels(), index);
			const glm::vec3 near_color = glm::vec3(foreground) / std::max(1.0f, foreground.a);
			preview_color::encode_pixel(near_color + far_color * (1.0f - alpha), index, color);
			coverage[index] = static_cast<std::uint8_t>(std::lround(alpha * 255.0f));
		}
	}
	return {PreviewImage(width, height, std::move(color)), std::move(coverage)};
}
