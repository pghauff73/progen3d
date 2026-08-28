#pragma once

#include <string>
#include <utility>
#include <vector>

class VegetationEvidenceProfile
{
public:
	VegetationEvidenceProfile(
		std::string geometry_authority,
		std::string biological_authority,
		std::string uncertainty_statement,
		std::vector<std::string> source_identifiers = {})
		: geometry_authority_(std::move(geometry_authority)),
		  biological_authority_(std::move(biological_authority)),
		  uncertainty_statement_(std::move(uncertainty_statement)),
		  source_identifiers_(std::move(source_identifiers))
	{
	}

	static VegetationEvidenceProfile architecturalArchetype()
	{
		return VegetationEvidenceProfile(
			"DeterministicGrammarAndAuthoredExtents",
			"ArchitecturalArchetypeOnly",
			"Taxon, specimen, optical, root, phenology, and biomechanical values remain unresolved until measurement evidence is attached.");
	}

	const std::string &geometryAuthority() const { return geometry_authority_; }
	const std::string &biologicalAuthority() const
	{
		return biological_authority_;
	}
	const std::string &uncertaintyStatement() const
	{
		return uncertainty_statement_;
	}
	const std::vector<std::string> &sourceIdentifiers() const
	{
		return source_identifiers_;
	}

private:
	std::string geometry_authority_;
	std::string biological_authority_;
	std::string uncertainty_statement_;
	std::vector<std::string> source_identifiers_;
};
