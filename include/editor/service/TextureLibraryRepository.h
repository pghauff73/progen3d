#pragma once

#include "BackendApiClient.h"

#include <cstddef>
#include <string>
#include <vector>

class TextureLibraryRepository
{
public:
	static constexpr std::size_t required_slot_count = 20;

	virtual ~TextureLibraryRepository() = default;

	virtual bool listSlots(std::vector<BackendTextureSlot> *slots,
	                       std::string *error_message) = 0;
	virtual bool fetchPreviewImage(const std::string &slot,
	                               std::string *png_bytes,
	                               std::string *error_message) = 0;
	virtual bool updateSlot(const std::string &slot,
	                       const std::string &display_name,
	                       float alpha,
	                       std::vector<BackendTextureSlot> *slots,
	                       std::string *error_message) = 0;
	virtual bool deleteSlot(const std::string &slot,
	                       std::vector<BackendTextureSlot> *slots,
	                       std::string *error_message) = 0;
	virtual bool uploadSlot(const std::string &slot,
	                       const std::string &display_name,
	                       float alpha,
	                       const std::string &local_path,
	                       std::vector<BackendTextureSlot> *slots,
	                       std::string *error_message) = 0;
	virtual bool generateSlot(const std::string &slot,
	                         const std::string &display_name,
	                         float alpha,
	                         const std::string &prompt,
	                         std::vector<BackendTextureSlot> *slots,
	                         BackendCreditSummary *credits,
	                         std::string *error_message) = 0;
};
