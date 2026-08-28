#include "building/service/SmallModernBuildingDeterministicHashService.h"

#include "building/model/BuildingClassificationModel.h"
#include "building/model/BuildingEvidenceLedger.h"
#include "building/model/BuildingFunctionModel.h"
#include "building/model/BuildingRequirementModel.h"
#include "building/model/BuildingRelationshipAssertionModel.h"
#include "building/model/BuildingScenarioModel.h"
#include "building/model/BuildingServiceModel.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <cstdint>
#include <string>

namespace {

class AggregateBuildingHashAccumulator {
public:
	void appendByte(std::uint8_t byte)
	{
		hash_ ^= byte;
		hash_ *= 1099511628211ULL;
	}

	void appendUnsigned(std::uint64_t value)
	{
		for (int byte_index = 0; byte_index < 8; ++byte_index) {
			appendByte(static_cast<std::uint8_t>((value >> (byte_index * 8)) & 0xffu));
		}
	}

	void appendString(const std::string &value)
	{
		appendUnsigned(value.size());
		for (unsigned char character : value) appendByte(character);
	}

	std::uint64_t value() const { return hash_; }

private:
	std::uint64_t hash_ = 14695981039346656037ULL;
};

} // namespace

std::uint64_t SmallModernBuildingDeterministicHashService::calculate(
	const SpatialBuildingModel &spatial_model,
	const BuildingClassificationModel &classification_model,
	const BuildingFunctionModel &function_model,
	const BuildingServiceModel &service_model,
	const BuildingRelationshipAssertionModel &relationship_assertion_model,
	const BuildingRequirementModel &requirement_model,
	const BuildingScenarioModel &scenario_model,
	const BuildingEvidenceLedger &evidence_ledger) const
{
	AggregateBuildingHashAccumulator hash;
	hash.appendString("SMB-OMv2.1-SmallModernBuildingModel-v1");
	hash.appendUnsigned(spatial_model.objects().size());
	for (const SpatialBuildingObject &object : spatial_model.objects().objects()) {
		hash.appendString(object.identity().objectId().value());
		hash.appendString(object.identity().objectClass().value());
	}
	hash.appendUnsigned(classification_model.concepts().size());
	for (const BuildingConceptDefinition &concept_definition :
	     classification_model.concepts()) {
		hash.appendString(concept_definition.conceptId().value());
		hash.appendString(concept_definition.canonicalName());
	}
	hash.appendUnsigned(classification_model.roles().size());
	for (const BuildingRoleDefinition &role : classification_model.roles()) {
		hash.appendString(role.roleId().value());
		hash.appendString(role.canonicalName());
	}
	hash.appendUnsigned(classification_model.objectProfiles().size());
	for (const BuildingObjectSemanticProfile &profile :
	     classification_model.objectProfiles()) {
		hash.appendString(profile.objectId().value());
		hash.appendUnsigned(profile.profileHash());
	}
	hash.appendUnsigned(function_model.functions().size());
	for (const BuildingFunction &function : function_model.functions()) {
		hash.appendString(function.functionId().value());
		hash.appendUnsigned(static_cast<std::uint64_t>(function.criticality()));
	}
	hash.appendUnsigned(function_model.allocations().size());
	for (const BuildingFunctionAllocationRelationship &allocation :
	     function_model.allocations()) {
		hash.appendString(allocation.allocationId().value());
		hash.appendString(allocation.functionId().value());
		hash.appendString(allocation.objectId().value());
	}
	hash.appendUnsigned(service_model.systems().size());
	for (const BuildingServiceSystem &system : service_model.systems()) {
		hash.appendString(system.systemId().value());
		hash.appendString(system.ownerObjectId().value());
	}
	hash.appendUnsigned(service_model.ports().size());
	for (const BuildingServicePort &port : service_model.ports()) {
		hash.appendString(port.portId().value());
		hash.appendString(port.ownerObjectId().value());
		hash.appendUnsigned(static_cast<std::uint64_t>(port.medium()));
		hash.appendUnsigned(static_cast<std::uint64_t>(port.direction()));
		hash.appendUnsigned(static_cast<std::uint64_t>(port.applicability()));
	}
	hash.appendUnsigned(service_model.flows().size());
	for (const BuildingServiceFlow &flow : service_model.flows()) {
		hash.appendString(flow.flowId().value());
		hash.appendString(flow.sourcePortId().value());
		hash.appendString(flow.targetPortId().value());
	}
	hash.appendUnsigned(relationship_assertion_model.assertions().size());
	for (const BuildingRelationshipAssertionReference &assertion :
	     relationship_assertion_model.assertions()) {
		hash.appendString(assertion.assertionObjectId().value());
		hash.appendUnsigned(static_cast<std::uint64_t>(assertion.assertionKind()));
		hash.appendString(assertion.canonicalFactId().value());
		hash.appendString(assertion.sourceObjectId().value());
		hash.appendString(assertion.targetObjectId().value());
	}
	hash.appendUnsigned(requirement_model.requirements().size());
	for (const auto &requirement : requirement_model.requirements()) {
		if (!requirement) continue;
		hash.appendString(requirement->requirementId().value());
		hash.appendUnsigned(static_cast<std::uint64_t>(requirement->kind()));
		hash.appendUnsigned(static_cast<std::uint64_t>(requirement->criticality()));
		hash.appendString(requirement->target().identifier());
	}
	hash.appendUnsigned(requirement_model.evaluationRecords().size());
	for (const BuildingRequirementEvaluationRecord &evaluation :
	     requirement_model.evaluationRecords()) {
		hash.appendString(evaluation.requirementId().value());
		hash.appendUnsigned(evaluation.evidenceHash());
	}
	hash.appendUnsigned(scenario_model.scenarios().size());
	for (const BuildingScenario &scenario : scenario_model.scenarios()) {
		hash.appendString(scenario.scenarioId().value());
	}
	hash.appendUnsigned(scenario_model.snapshots().size());
	for (const BuildingStateSnapshot &snapshot : scenario_model.snapshots()) {
		hash.appendString(snapshot.scenarioId().value());
		hash.appendUnsigned(snapshot.snapshotHash());
	}
	hash.appendUnsigned(evidence_ledger.records().size());
	for (const BuildingEvidenceRecord &evidence : evidence_ledger.records()) {
		hash.appendString(evidence.evidenceId().value());
		hash.appendUnsigned(evidence.evidenceHash());
	}
	return hash.value();
}
