#pragma once

#include <string>
#include <utility>
#include <vector>

class ModernCarAssuranceLevel
{
public:
	ModernCarAssuranceLevel(
		std::string identifier,
		bool passed,
		bool partial_static_envelopes,
		std::string claim,
		std::string note)
		: identifier_(std::move(identifier)),
		  passed_(passed),
		  partial_static_envelopes_(partial_static_envelopes),
		  claim_(std::move(claim)),
		  note_(std::move(note))
	{
	}

	const std::string &identifier() const { return identifier_; }
	bool passed() const { return passed_; }
	bool partialStaticEnvelopes() const { return partial_static_envelopes_; }
	const std::string &claim() const { return claim_; }
	const std::string &note() const { return note_; }

private:
	std::string identifier_;
	bool passed_ = false;
	bool partial_static_envelopes_ = false;
	std::string claim_;
	std::string note_;
};

class ModernCarSourceAssurance
{
public:
	ModernCarSourceAssurance(
		std::string schema,
		std::string achieved_level,
		std::string achieved_label,
		bool release_gate_passed,
		std::vector<ModernCarAssuranceLevel> levels,
		std::vector<std::string> known_limitations)
		: schema_(std::move(schema)),
		  achieved_level_(std::move(achieved_level)),
		  achieved_label_(std::move(achieved_label)),
		  release_gate_passed_(release_gate_passed),
		  levels_(std::move(levels)),
		  known_limitations_(std::move(known_limitations))
	{
	}

	const std::string &schema() const { return schema_; }
	const std::string &achievedLevel() const { return achieved_level_; }
	const std::string &achievedLabel() const { return achieved_label_; }
	bool releaseGatePassed() const { return release_gate_passed_; }
	const std::vector<ModernCarAssuranceLevel> &levels() const { return levels_; }
	const std::vector<std::string> &knownLimitations() const
	{
		return known_limitations_;
	}

	const ModernCarAssuranceLevel *findLevel(const std::string &identifier) const
	{
		for (const ModernCarAssuranceLevel &level : levels_) {
			if (level.identifier() == identifier) return &level;
		}
		return nullptr;
	}

private:
	std::string schema_;
	std::string achieved_level_;
	std::string achieved_label_;
	bool release_gate_passed_ = false;
	std::vector<ModernCarAssuranceLevel> levels_;
	std::vector<std::string> known_limitations_;
};
