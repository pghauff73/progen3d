#include "DepthOfField.h"

#include "PreviewColor.h"
#include "editor/service/RenderRowExecutor.h"

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

// Add a disk as horizontal intervals in either the colour or coverage buffer.
template <typename Pixel, typename ReadPixel>
std::vector<Pixel> scatter_disks(RenderRowExecutor &rows, int width, int height, const std::vector<std::uint8_t> &radii,
                                int max_radius, ReadPixel read_pixel)
{
	const std::size_t stride = static_cast<std::size_t>(width) + 1;
	std::vector<Pixel> sums(stride * height, Pixel(0.0f));
	const auto kernels = disk_kernels(max_radius);
	// A band reads its surrounding source rows and writes only its own intervals.
	// Each destination row receives contributions in the original source order.
	const auto bands = static_cast<unsigned>(std::clamp(height / (2 * max_radius + 64), 1, 8));
	rows.run(width, height, [&](int first_row, int end_row) {
		for (int y = first_row - max_radius; y < end_row + max_radius; ++y) {
			for (int x = -max_radius; x < width + max_radius; ++x) {
				// Extend the image edges so a foreground object can fill the picture edge.
				const std::size_t source = static_cast<std::size_t>(std::clamp(y, 0, height - 1)) * width +
				                           std::clamp(x, 0, width - 1);
				const int radius = radii[source];
				if (radius == 0) {
					continue;
				}
				const DiskKernel &kernel = kernels[radius];
				const Pixel contribution = read_pixel(source) * kernel.weight;
				for (int row = std::max(first_row, y - radius); row <= std::min(end_row - 1, y + radius); ++row) {
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
	}, bands);
	return sums;
}

std::vector<glm::vec4> scatter_color(RenderRowExecutor &rows, const PreviewImage &image,
                                     const std::vector<std::uint8_t> &radii, int max_radius)
{
	return scatter_disks<glm::vec4>(rows, image.width(), image.height(), radii, max_radius,
		[&](std::size_t source) {
			return glm::vec4(preview_color::decode_pixel(image.rgbPixels(), source), 1.0f);
		});
}

}

DepthOfField::DepthOfField(const ShotCameraSample &shot, int image_height, RenderRowExecutor &rows)
	: rows_(rows), blur_scale_(static_cast<double>(image_height) * shot.lens.aperture_radius /
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

int DepthOfField::foregroundRadius(float depth, float cutoff) const
{
	return (depth > 0.0f && depth < cutoff) ? static_cast<int>(std::lround(blurRadius(depth))) : 0;
}

std::vector<std::uint8_t> DepthOfField::foregroundCoverage(int width, int height,
                                                          const std::vector<float> &sharp_depth) const
{
	const float cutoff = foregroundCutoff();
	std::vector<std::uint8_t> radii(sharp_depth.size(), 0);
	int max_radius = 0;
	for (std::size_t index = 0; index < sharp_depth.size(); ++index) {
		const int radius = foregroundRadius(sharp_depth[index], cutoff);
		radii[index] = static_cast<std::uint8_t>(radius);
		max_radius = std::max(max_radius, radius);
	}
	std::vector<std::uint8_t> coverage(sharp_depth.size(), 0);
	if (max_radius == 0) return coverage;
	const auto near_sums = scatter_disks<float>(rows_, width, height, radii, max_radius,
	                                           [](std::size_t) { return 1.0f; });
	rows_.run(width, height, [&](int first_row, int end_row) {
		for (int row = first_row; row < end_row; ++row) {
			const auto offset = static_cast<std::size_t>(row) * width;
			float alpha = 0.0f;
			for (int x = 0; x < width; ++x) {
				alpha += near_sums[offset + row + x];
				coverage[offset + x] = static_cast<std::uint8_t>(std::lround(std::clamp(alpha, 0.0f, 1.0f) * 255.0f));
			}
		}
	});
	return coverage;
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
		const int near_radius = foregroundRadius(sharp_depth[index], cutoff);
		near_radii[index] = static_cast<std::uint8_t>(near_radius);
		max_near_radius = std::max(max_near_radius, near_radius);
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
		near_sums = scatter_color(rows_, sharp, near_radii, max_near_radius);
	}
	std::vector<glm::vec4> far_sums;
	if (max_far_radius > 0) {
		far_sums = scatter_color(rows_, far_plate, far_radii, max_far_radius);
	}

	std::vector<std::uint8_t> color = far_plate.rgbPixels();
	std::vector<std::uint8_t> coverage(pixel_count, 0);
	rows_.run(width, height, [&](int first_row, int end_row) {
		for (int y = first_row; y < end_row; ++y) {
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
	});
	return {PreviewImage(width, height, std::move(color)), std::move(coverage)};
}
