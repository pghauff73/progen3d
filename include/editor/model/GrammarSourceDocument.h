#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

class GrammarSourceDocument
{
public:
	std::string source_text;
	bool dirty = false;
	std::string title = "Untitled grammar";
	std::filesystem::path local_path;
	std::uint64_t design_nonce = 0;
	std::uint64_t last_successful_generation_request_id = 0;

	void replaceSourceText(std::string source_text_value, bool is_dirty)
	{
		source_text = std::move(source_text_value);
		dirty = is_dirty;
	}

	void markEdited()
	{
		dirty = true;
	}

	void markPersisted()
	{
		dirty = false;
	}

	void clearLocalIdentity()
	{
		local_path.clear();
		title = "Untitled grammar";
	}

	void adoptLocalIdentity(const std::filesystem::path &path)
	{
		local_path = path;
		if (!path.filename().empty()) {
			title = path.filename().string();
		}
	}

	void recordSuccessfulGeneration(std::uint64_t request_id,
	                                std::uint64_t generated_design_nonce)
	{
		last_successful_generation_request_id = request_id;
		design_nonce = generated_design_nonce;
	}
};
