#include "vehicle/mcsmv2/service/VehicleBodyInWhiteValidationService.h"

#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

ModernCarSemanticValidationReport VehicleBodyInWhiteValidationService::validate(
	const VehicleBodyInWhiteAssembly &assembly) const
{
	ModernCarSemanticValidationReport report;
	std::set<std::string> identifiers;
	std::map<std::string, std::vector<std::string>> adjacency;
	for (const BodyInWhiteStructuralMember &member : assembly.members()) {
		if (member.identifier().empty() || member.role().empty() ||
		    !identifiers.insert(member.identifier()).second) {
			report.addIssue({
				ModernCarSemanticValidationCode::DisconnectedBodyInWhite,
				"Body-in-white members must have unique identifiers and explicit roles.",
				{assembly.variantIdentifier(), member.identifier()}});
		}
		adjacency[member.identifier()];
	}
	std::set<std::string> datum_identifiers;
	for (const BodyInWhiteJointRelationship &joint : assembly.joints()) {
		if (identifiers.count(joint.firstMemberIdentifier()) == 0u ||
		    identifiers.count(joint.secondMemberIdentifier()) == 0u ||
		    joint.firstMemberIdentifier() == joint.secondMemberIdentifier() ||
		    joint.joiningMethod().empty() || joint.sharedDatumIdentifier().empty() ||
		    !datum_identifiers.insert(joint.sharedDatumIdentifier()).second) {
			report.addIssue({
				ModernCarSemanticValidationCode::DisconnectedBodyInWhite,
				"Body-in-white joints must associate two known members through one unique shared datum.",
				{assembly.variantIdentifier(), joint.firstMemberIdentifier(),
				 joint.secondMemberIdentifier()}});
			continue;
		}
		adjacency[joint.firstMemberIdentifier()].push_back(
			joint.secondMemberIdentifier());
		adjacency[joint.secondMemberIdentifier()].push_back(
			joint.firstMemberIdentifier());
	}
	if (!identifiers.empty()) {
		std::set<std::string> reached;
		std::queue<std::string> pending;
		pending.push(*identifiers.begin());
		while (!pending.empty()) {
			const std::string identifier = pending.front();
			pending.pop();
			if (!reached.insert(identifier).second) continue;
			for (const std::string &neighbor : adjacency[identifier]) {
				pending.push(neighbor);
			}
		}
		if (reached.size() != identifiers.size()) {
			report.addIssue({
				ModernCarSemanticValidationCode::DisconnectedBodyInWhite,
				"Body-in-white structural graph is disconnected.",
				{assembly.variantIdentifier()}});
		}
	}
	return report;
}
