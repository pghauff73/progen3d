#pragma once

#include <filesystem>
#include <string>

class PartCatalogRepository
{
public:
	virtual ~PartCatalogRepository() = default;

	virtual bool loadPartClassCatalog(const std::filesystem::path &catalog_path,
	                                  std::string *error_message) = 0;
};
