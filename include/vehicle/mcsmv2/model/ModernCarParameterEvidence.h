#pragma once

#include <optional>
#include <string>
#include <utility>

class ModernCarParameterEvidence
{
public:
	ModernCarParameterEvidence(
		std::string status,
		std::string source,
		double confidence,
		std::optional<double> uncertainty,
		std::string note)
		: status_(std::move(status)),
		  source_(std::move(source)),
		  confidence_(confidence),
		  uncertainty_(uncertainty),
		  note_(std::move(note))
	{
	}

	const std::string &status() const { return status_; }
	const std::string &source() const { return source_; }
	double confidence() const { return confidence_; }
	const std::optional<double> &uncertainty() const { return uncertainty_; }
	const std::string &note() const { return note_; }

private:
	std::string status_;
	std::string source_;
	double confidence_ = 0.0;
	std::optional<double> uncertainty_;
	std::string note_;
};
