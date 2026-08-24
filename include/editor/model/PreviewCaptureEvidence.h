#pragma once

#include <cstdint>
#include <string>
#include <utility>

class PreviewCaptureEvidence
{
public:
	static PreviewCaptureEvidence createFailure(std::string message)
	{
		PreviewCaptureEvidence evidence;
		evidence.message_ = std::move(message);
		return evidence;
	}

	static PreviewCaptureEvidence createSuccess(std::string path,
	                                           int width,
	                                           int height,
	                                           std::uint64_t pixel_hash)
	{
		PreviewCaptureEvidence evidence;
		evidence.succeeded_ = true;
		evidence.output_path_ = std::move(path);
		evidence.width_ = width;
		evidence.height_ = height;
		evidence.pixel_hash_ = pixel_hash;
		return evidence;
	}

	bool succeeded() const { return succeeded_; }
	const std::string &message() const { return message_; }
	const std::string &outputPath() const { return output_path_; }
	int width() const { return width_; }
	int height() const { return height_; }
	std::uint64_t pixelHash() const { return pixel_hash_; }

private:
	bool succeeded_ = false;
	std::string message_;
	std::string output_path_;
	int width_ = 0;
	int height_ = 0;
	std::uint64_t pixel_hash_ = 0;
};
