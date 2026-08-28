#pragma once

#include <string>
#include <utility>

class DocumentStorageIdentity
{
public:
	std::string cloud_file_id;
	std::string cloud_title;
	bool cloud_published = false;
	std::string remote_updated_at;

	bool hasCloudIdentity() const
	{
		return !cloud_file_id.empty();
	}

	void clearCloudIdentity()
	{
		cloud_file_id.clear();
		cloud_title.clear();
		cloud_published = false;
		remote_updated_at.clear();
	}

	void adoptCloudIdentity(std::string file_id,
	                        std::string title,
	                        bool published,
	                        std::string updated_at = {})
	{
		cloud_file_id = std::move(file_id);
		cloud_title = std::move(title);
		cloud_published = published;
		remote_updated_at = std::move(updated_at);
	}

	void markUnpublished()
	{
		cloud_published = false;
	}
};
