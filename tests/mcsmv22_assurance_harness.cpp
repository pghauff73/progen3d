#include "vehicle/mcsmv2/service/McsMv22KinematicAssuranceEvaluationService.h"

#include <iostream>
#include <string>

namespace {

bool require(bool condition, const std::string &message)
{
	if (!condition) std::cerr << "FAIL: " << message << '\n';
	return condition;
}

} // namespace

int main()
{
	const McsMv22KinematicAssuranceEvaluationService service;
	bool passed = true;
	const McsMv22KinematicAssuranceReport accepted = service.evaluate(
		"reference", true, true, true, true, true, true, true, true, true);
	passed &= require(accepted.achievedV3(), "all explicit V3 gates must achieve V3");
	passed &= require(accepted.levels().size() == 6u, "assurance report must expose V0 through V5");
	passed &= require(accepted.diagnostics().empty(), "accepted assurance must have no diagnostics");
	passed &= require(accepted.findLevel("V4") != nullptr && !accepted.findLevel("V4")->passed(), "V4 must remain false");
	passed &= require(accepted.findLevel("V5") != nullptr && !accepted.findLevel("V5")->passed(), "V5 must remain false");

	const McsMv22KinematicAssuranceReport rejected = service.evaluate(
		"reference", true, true, true, true, true, true, true, false, true);
	passed &= require(!rejected.achievedV3(), "a failed glass binding must fail V3 closed");
	passed &= require(!rejected.diagnostics().empty(), "a rejected assurance report must explain the failed gate");
	passed &= require(rejected.findLevel("V2") != nullptr && rejected.findLevel("V2")->passed(), "a V3 failure must not erase an accepted V2 claim");

	if (!passed) return 1;
	std::cout << "MCSMv2.2 assurance checks passed.\n";
	return 0;
}
