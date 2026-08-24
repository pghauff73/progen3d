#pragma once

#include "geometry/model/ShapeSpecification.h"

#include <string>
#include <utility>
#include <vector>

class ProceduralShapeParameterDocumentation
{
public:
	ProceduralShapeParameterDocumentation(std::string name,
	                                      std::string signature,
	                                      std::string default_value,
	                                      std::string description)
		: name_(std::move(name)),
		  signature_(std::move(signature)),
		  default_value_(std::move(default_value)),
		  description_(std::move(description))
	{
	}

	const std::string &name() const { return name_; }
	const std::string &signature() const { return signature_; }
	const std::string &defaultValue() const { return default_value_; }
	const std::string &description() const { return description_; }

private:
	std::string name_;
	std::string signature_;
	std::string default_value_;
	std::string description_;
};

class ProceduralShapeCatalogEntry
{
public:
	ProceduralShapeCatalogEntry(std::string identifier,
	                            std::string display_name,
	                            ShapeFamily family,
	                            bool alias,
	                            std::string canonical_expansion,
	                            std::string summary,
	                            std::vector<ProceduralShapeParameterDocumentation> parameters)
		: identifier_(std::move(identifier)),
		  display_name_(std::move(display_name)),
		  family_(family),
		  alias_(alias),
		  canonical_expansion_(std::move(canonical_expansion)),
		  summary_(std::move(summary)),
		  parameters_(std::move(parameters))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &displayName() const { return display_name_; }
	ShapeFamily family() const { return family_; }
	bool isAlias() const { return alias_; }
	const std::string &canonicalExpansion() const { return canonical_expansion_; }
	const std::string &summary() const { return summary_; }
	const std::vector<ProceduralShapeParameterDocumentation> &parameters() const
	{
		return parameters_;
	}

private:
	std::string identifier_;
	std::string display_name_;
	ShapeFamily family_ = ShapeFamily::Cylinder;
	bool alias_ = false;
	std::string canonical_expansion_;
	std::string summary_;
	std::vector<ProceduralShapeParameterDocumentation> parameters_;
};
