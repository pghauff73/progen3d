#pragma once

#include "vegetation/model/VegetationMeasuredSourceSchemaRegistration.h"

#include <optional>
#include <string>
#include <vector>

class VegetationMeasuredSourceSchemaRegistry
{
public:
	VegetationMeasuredSourceSchemaRegistry();
	explicit VegetationMeasuredSourceSchemaRegistry(
		std::vector<VegetationMeasuredSourceSchemaRegistration> registrations);

	const std::vector<VegetationMeasuredSourceSchemaRegistration> &registrations()
		const
	{
		return registrations_;
	}
	std::vector<VegetationMeasuredSourceSchemaRegistration> findRegistrations(
		const std::string &source_media_type,
		const std::string &source_schema_version) const;
	std::optional<VegetationMeasuredSourceSchemaRegistration> resolveUnique(
		const std::string &source_media_type,
		const std::string &source_schema_version) const;

private:
	std::vector<VegetationMeasuredSourceSchemaRegistration> registrations_;
};
