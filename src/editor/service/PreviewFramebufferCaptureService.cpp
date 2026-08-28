#include "editor/service/PreviewFramebufferCaptureService.h"

#include "editor/model/PreviewImage.h"
#include "imgui_render.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

std::uint64_t calculate_pixel_hash(const PreviewImage &image)
{
	constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
	constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
	std::uint64_t hash = kFnvOffsetBasis;
	for (const std::uint8_t channel : image.rgbPixels()) {
		hash ^= static_cast<std::uint64_t>(channel);
		hash *= kFnvPrime;
	}
	return hash;
}

}

PreviewCaptureEvidence PreviewFramebufferCaptureService::captureCurrentPreview(
	const std::string &output_path) const
{
	if (output_path.empty()) {
		return PreviewCaptureEvidence::createFailure(
			"Preview capture requires an output path.");
	}

	std::vector<std::uint8_t> pixels;
	int width = 0;
	int height = 0;
	if (!read_preview_rgb_pixels(&pixels, &width, &height)) {
		return PreviewCaptureEvidence::createFailure(
			"The preview framebuffer could not be read.");
	}
	const PreviewImage image(width, height, std::move(pixels));
	if (!image.isValid()) {
		return PreviewCaptureEvidence::createFailure(
			"The preview framebuffer returned invalid pixel dimensions.");
	}

	const std::filesystem::path path(output_path);
	std::error_code directory_error;
	if (!path.parent_path().empty()) {
		std::filesystem::create_directories(path.parent_path(), directory_error);
	}
	if (directory_error) {
		return PreviewCaptureEvidence::createFailure(
			"The preview capture directory could not be created: " +
			directory_error.message());
	}

	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	if (!output) {
		return PreviewCaptureEvidence::createFailure(
			"The preview capture file could not be opened: " + output_path);
	}
	output << "P6\n" << image.width() << ' ' << image.height() << "\n255\n";
	output.write(reinterpret_cast<const char *>(image.rgbPixels().data()),
	             static_cast<std::streamsize>(image.rgbPixels().size()));
	if (!output) {
		return PreviewCaptureEvidence::createFailure(
			"The preview capture file could not be written: " + output_path);
	}

	return PreviewCaptureEvidence::createSuccess(
		output_path, image.width(), image.height(), calculate_pixel_hash(image));
}
