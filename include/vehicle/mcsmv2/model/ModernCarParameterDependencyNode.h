#pragma once

#include "vehicle/mcsmv2/model/ModernCarParameterEvidence.h"

#include <string>
#include <utility>
#include <vector>

class ModernCarParameterDependencyNode
{
public:
	ModernCarParameterDependencyNode(
		std::string identifier,
		std::string serialized_value,
		std::string unit,
		std::string value_type,
		std::string formula,
		std::vector<std::string> dependencies,
		ModernCarParameterEvidence evidence)
		: identifier_(std::move(identifier)),
		  serialized_value_(std::move(serialized_value)),
		  unit_(std::move(unit)),
		  value_type_(std::move(value_type)),
		  formula_(std::move(formula)),
		  dependencies_(std::move(dependencies)),
		  evidence_(std::move(evidence))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &serializedValue() const { return serialized_value_; }
	const std::string &unit() const { return unit_; }
	const std::string &valueType() const { return value_type_; }
	const std::string &formula() const { return formula_; }
	const std::vector<std::string> &dependencies() const { return dependencies_; }
	const ModernCarParameterEvidence &evidence() const { return evidence_; }

private:
	std::string identifier_;
	std::string serialized_value_;
	std::string unit_;
	std::string value_type_;
	std::string formula_;
	std::vector<std::string> dependencies_;
	ModernCarParameterEvidence evidence_{"", "", 0.0, std::nullopt, ""};
};
