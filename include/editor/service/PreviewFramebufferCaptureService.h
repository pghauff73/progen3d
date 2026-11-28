#pragma once

#include "editor/model/PreviewCaptureEvidence.h"

#include <filesystem>
#include <iosfwd>
#include <string>

class PreviewImage;

class PreviewFramebufferCaptureService
{
public:
	PreviewCaptureEvidence captureCurrentPreview(const std::string &output_path) const;
	PreviewCaptureEvidence writePreview(const PreviewImage &image,
	                                    const std::filesystem::path &path) const;
	static void writePpm(const PreviewImage &image, std::ostream &output);
	static std::uint64_t pixelHash(const PreviewImage &image);
};
