#pragma once

#include "editor/model/PreviewCaptureEvidence.h"

#include <string>

class PreviewFramebufferCaptureService
{
public:
	PreviewCaptureEvidence captureCurrentPreview(const std::string &output_path) const;
};
