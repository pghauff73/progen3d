#pragma once

#include "editor/service/TextureLibraryRepository.h"

class BackendTextureLibraryRepository final : public TextureLibraryRepository
{
public:
	BackendTextureLibraryRepository(const FirebaseAuthConfig &configuration,
	                                FirebaseAuthSession &session);

	bool listSlots(std::vector<BackendTextureSlot> *slots,
	               std::string *error_message) override;
	bool fetchPreviewImage(const std::string &slot,
	                       std::string *png_bytes,
	                       std::string *error_message) override;
	bool updateSlot(const std::string &slot,
	               const std::string &display_name,
	               float alpha,
	               std::vector<BackendTextureSlot> *slots,
	               std::string *error_message) override;
	bool deleteSlot(const std::string &slot,
	               std::vector<BackendTextureSlot> *slots,
	               std::string *error_message) override;
	bool uploadSlot(const std::string &slot,
	               const std::string &display_name,
	               float alpha,
	               const std::string &local_path,
	               std::vector<BackendTextureSlot> *slots,
	               std::string *error_message) override;
	bool generateSlot(const std::string &slot,
	                 const std::string &display_name,
	                 float alpha,
	                 const std::string &prompt,
	                 std::vector<BackendTextureSlot> *slots,
	                 BackendCreditSummary *credits,
	                 std::string *error_message) override;

private:
	void normalizeSlotCollection(std::vector<BackendTextureSlot> *slots) const;

	FirebaseAuthConfig configuration_;
	FirebaseAuthSession &session_;
};
