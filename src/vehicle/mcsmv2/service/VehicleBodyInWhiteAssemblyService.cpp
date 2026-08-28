#include "vehicle/mcsmv2/service/VehicleBodyInWhiteAssemblyService.h"

#include "vehicle/mcsmv2/generated/GeneratedMcsMv2Catalog.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

const GeneratedMcsMv2VariantRecord &sourceRecordFor(
	const std::string &variant_identifier)
{
	for (const GeneratedMcsMv2VariantRecord &record :
	     generatedMcsMv2VariantRecords()) {
		if (record.key == variant_identifier) return record;
	}
	throw std::invalid_argument(
		"No generated MCSMv2 source record exists for variant '" +
		variant_identifier + "'.");
}

BodyInWhiteMemberGeometryFamily geometryFamilyForRole(const std::string &role)
{
	if (role == "floor_pan") return BodyInWhiteMemberGeometryFamily::FormedPanel;
	if (role.find("tower") != std::string::npos) {
		return BodyInWhiteMemberGeometryFamily::CompoundShape;
	}
	return BodyInWhiteMemberGeometryFamily::VariableSectionSweep;
}

} // namespace

VehicleBodyInWhiteAssembly VehicleBodyInWhiteAssemblyService::assemble(
	const ModernCarSemanticVariant &variant) const
{
	const GeneratedMcsMv2VariantRecord &record =
		sourceRecordFor(variant.identifier());
	std::vector<BodyInWhiteStructuralMember> members;
	members.reserve(record.body_in_white_members.size());
	for (const GeneratedMcsMv2BodyInWhiteMemberRecord &member :
	     record.body_in_white_members) {
		members.emplace_back(
			std::string(member.identifier),
			std::string(member.role),
			geometryFamilyForRole(std::string(member.role)));
	}
	std::vector<BodyInWhiteJointRelationship> joints;
	joints.reserve(record.body_in_white_joints.size());
	std::size_t joint_index = 0u;
	for (const GeneratedMcsMv2BodyInWhiteJointRecord &joint :
	     record.body_in_white_joints) {
		joints.emplace_back(
			std::string(joint.first_member_identifier),
			std::string(joint.second_member_identifier),
			std::string(joint.joining_method),
			variant.identifier() + ".BIW.SharedDatum." +
				std::to_string(joint_index++));
	}
	return VehicleBodyInWhiteAssembly(
		variant.identifier(), std::move(members), std::move(joints));
}
