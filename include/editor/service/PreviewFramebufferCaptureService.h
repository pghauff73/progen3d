#pragma once

#include "editor/model/PreviewCaptureEvidence.h"

#include <filesystem>
#include <string>

class PreviewImage;

class PreviewFramebufferCaptureService
{
public:
	PreviewCaptureEvidence captureCurrentPreview(const std::string &output_path) const;
	PreviewCaptureEvidence writePreview(const PreviewImage &image,
	                                    const std::filesystem::path &path) const;
};
