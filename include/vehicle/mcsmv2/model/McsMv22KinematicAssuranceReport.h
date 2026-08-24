#pragma once

#include <string>
#include <utility>
#include <vector>

class McsMv22AssuranceLevelStatus
{
public:
	McsMv22AssuranceLevelStatus(
		std::string identifier,
		bool passed,
		std::string claim,
		std::string boundary)
		: identifier_(std::move(identifier)),
		  passed_(passed),
		  claim_(std::move(claim)),
		  boundary_(std::move(boundary))
	{
	}

	const std::string &identifier() const { return identifier_; }
	bool passed() const { return passed_; }
	const std::string &claim() const { return claim_; }
	const std::string &boundary() const { return boundary_; }

private:
	std::string identifier_;
	bool passed_ = false;
	std::string claim_;
	std::string boundary_;
};

class McsMv22KinematicAssuranceReport
{
public:
	McsMv22KinematicAssuranceReport(
		std::string variant_identifier,
		std::vector<McsMv22AssuranceLevelStatus> levels,
		std::vector<std::string> diagnostics)
		: variant_identifier_(std::move(variant_identifier)),
		  levels_(std::move(levels)),
		  diagnostics_(std::move(diagnostics))
	{
	}

	const std::string &variantIdentifier() const { return variant_identifier_; }
	const std::vector<McsMv22AssuranceLevelStatus> &levels() const
	{
		return levels_;
	}
	const std::vector<std::string> &diagnostics() const { return diagnostics_; }
	const McsMv22AssuranceLevelStatus *findLevel(
		const std::string &identifier) const
	{
		for (const McsMv22AssuranceLevelStatus &level : levels_) {
			if (level.identifier() == identifier) return &level;
		}
		return nullptr;
	}
	bool achievedV3() const
	{
		const McsMv22AssuranceLevelStatus *level = findLevel("V3");
		return level != nullptr && level->passed();
	}

private:
	std::string variant_identifier_;
	std::vector<McsMv22AssuranceLevelStatus> levels_;
	std::vector<std::string> diagnostics_;
};
