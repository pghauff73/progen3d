#include "editor/service/BackendTextureLibraryRepository.h"

#include <algorithm>

BackendTextureLibraryRepository::BackendTextureLibraryRepository(
	const FirebaseAuthConfig &configuration,
	FirebaseAuthSession &session)
	: configuration_(configuration),
	  session_(session)
{
}

bool BackendTextureLibraryRepository::listSlots(
	std::vector<BackendTextureSlot> *slots,
	std::string *error_message)
{
	if (!backend_api_list_textures(configuration_, &session_, slots, error_message)) {
		return false;
	}
	normalizeSlotCollection(slots);
	return true;
}

bool BackendTextureLibraryRepository::fetchPreviewImage(
	const std::string &slot,
	std::string *png_bytes,
	std::string *error_message)
{
	return backend_api_fetch_texture_image(configuration_,
	                                       &session_,
	                                       slot,
	                                       png_bytes,
	                                       error_message);
}

bool BackendTextureLibraryRepository::updateSlot(
	const std::string &slot,
	const std::string &display_name,
	float alpha,
	std::vector<BackendTextureSlot> *slots,
	std::string *error_message)
{
	if (!backend_api_update_texture(configuration_,
	                                &session_,
	                                slot,
	                                display_name,
	                                alpha,
	                                slots,
	                                error_message)) {
		return false;
	}
	normalizeSlotCollection(slots);
	return true;
}

bool BackendTextureLibraryRepository::deleteSlot(
	const std::string &slot,
	std::vector<BackendTextureSlot> *slots,
	std::string *error_message)
{
	if (!backend_api_delete_texture(configuration_, &session_, slot, slots, error_message)) {
		return false;
	}
	normalizeSlotCollection(slots);
	return true;
}

bool BackendTextureLibraryRepository::uploadSlot(
	const std::string &slot,
	const std::string &display_name,
	float alpha,
	const std::string &local_path,
	std::vector<BackendTextureSlot> *slots,
	std::string *error_message)
{
	if (!backend_api_upload_texture(configuration_,
	                                &session_,
	                                slot,
	                                display_name,
	                                alpha,
	                                local_path,
	                                slots,
	                                error_message)) {
		return false;
	}
	normalizeSlotCollection(slots);
	return true;
}

bool BackendTextureLibraryRepository::generateSlot(
	const std::string &slot,
	const std::string &display_name,
	float alpha,
	const std::string &prompt,
	std::vector<BackendTextureSlot> *slots,
	BackendCreditSummary *credits,
	std::string *error_message)
{
	if (!backend_api_generate_texture(configuration_,
	                                  &session_,
	                                  slot,
	                                  display_name,
	                                  alpha,
	                                  prompt,
	                                  slots,
	                                  credits,
	                                  error_message)) {
		return false;
	}
	normalizeSlotCollection(slots);
	return true;
}

void BackendTextureLibraryRepository::normalizeSlotCollection(
	std::vector<BackendTextureSlot> *slots) const
{
	if (slots == nullptr) {
		return;
	}
	std::vector<BackendTextureSlot> normalized_slots;
	normalized_slots.reserve(required_slot_count);
	for (std::size_t slot_number = 1; slot_number <= required_slot_count; ++slot_number) {
		const std::string slot_name = "usertexture" + std::to_string(slot_number);
		const auto matching_slot = std::find_if(
			slots->begin(),
			slots->end(),
			[&slot_name](const BackendTextureSlot &slot) { return slot.slot == slot_name; });
		if (matching_slot != slots->end()) {
			normalized_slots.push_back(*matching_slot);
		} else {
			BackendTextureSlot placeholder;
			placeholder.slot = slot_name;
			placeholder.display_name = slot_name;
			normalized_slots.push_back(std::move(placeholder));
		}
	}
	*slots = std::move(normalized_slots);
}
