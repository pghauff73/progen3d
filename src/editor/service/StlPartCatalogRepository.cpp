#include "editor/service/StlPartCatalogRepository.h"

#include "StlCatalog.h"

bool StlPartCatalogRepository::loadPartClassCatalog(
	const std::filesystem::path &catalog_path,
	std::string *error_message)
{
	if (load_part_class_catalog(catalog_path.string())) {
		if (error_message != nullptr) {
			error_message->clear();
		}
		return true;
	}

	if (error_message != nullptr) {
		*error_message = "Could not load part class catalog from " +
		                 catalog_path.string() + "; using built-in part classes.";
	}
	return false;
}
