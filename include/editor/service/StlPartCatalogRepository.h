#pragma once

#include "editor/service/PartCatalogRepository.h"

class StlPartCatalogRepository final : public PartCatalogRepository
{
public:
	bool loadPartClassCatalog(const std::filesystem::path &catalog_path,
	                          std::string *error_message) override;
};
