#pragma once

#include <string>
#include <utility>

class ArchitecturalReferenceIdentity
{
public:
	ArchitecturalReferenceIdentity(
		std::string reference_identifier,
		std::string revision_identifier,
		std::string source_location,
		std::string content_hash)
		: reference_identifier_(std::move(reference_identifier)),
		  revision_identifier_(std::move(revision_identifier)),
		  source_location_(std::move(source_location)),
		  content_hash_(std::move(content_hash))
	{
	}

	const std::string &referenceIdentifier() const
	{
		return reference_identifier_;
	}

	const std::string &revisionIdentifier() const
	{
		return revision_identifier_;
	}

	const std::string &sourceLocation() const
	{
		return source_location_;
	}

	const std::string &contentHash() const
	{
		return content_hash_;
	}

	bool isComplete() const
	{
		return !reference_identifier_.empty() && !revision_identifier_.empty() &&
		       !source_location_.empty() && !content_hash_.empty();
	}

private:
	std::string reference_identifier_;
	std::string revision_identifier_;
	std::string source_location_;
	std::string content_hash_;
};
