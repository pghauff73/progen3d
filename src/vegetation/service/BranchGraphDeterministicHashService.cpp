#include "vegetation/service/BranchGraphDeterministicHashService.h"

#include "vegetation/model/BranchGraph.h"

#include <cstddef>
#include <cstring>
#include <string>

namespace {

void hash_bytes(std::uint64_t *hash, const void *data, std::size_t size)
{
	const auto *bytes = static_cast<const unsigned char *>(data);
	for (std::size_t index = 0; index < size; ++index) {
		*hash ^= bytes[index];
		*hash *= 1099511628211ull;
	}
}

void hash_string(std::uint64_t *hash, const std::string &text)
{
	hash_bytes(hash, text.data(), text.size());
	const unsigned char separator = 0xffu;
	hash_bytes(hash, &separator, sizeof(separator));
}

} // namespace

std::uint64_t BranchGraphDeterministicHashService::hash(
	const BranchGraph &graph) const
{
	std::uint64_t value = 1469598103934665603ull;
	hash_string(&value, graph.rootNodeIdentifier());
	for (const BranchNode &node : graph.nodes()) {
		hash_string(&value, node.identifier());
		hash_bytes(&value, &node.position().x, sizeof(float));
		hash_bytes(&value, &node.position().y, sizeof(float));
		hash_bytes(&value, &node.position().z, sizeof(float));
		const float radius = node.radius();
		const float age = node.developmentalAge();
		const int order = node.branchOrder();
		const int state = static_cast<int>(node.state());
		hash_bytes(&value, &radius, sizeof(radius));
		hash_bytes(&value, &age, sizeof(age));
		hash_bytes(&value, &order, sizeof(order));
		hash_bytes(&value, &state, sizeof(state));
	}
	for (const BranchSegment &segment : graph.segments()) {
		hash_string(&value, segment.identifier());
		hash_string(&value, segment.parentNodeIdentifier());
		hash_string(&value, segment.childNodeIdentifier());
		const int kind = static_cast<int>(segment.kind());
		hash_bytes(&value, &kind, sizeof(kind));
	}
	for (const OrganAttachment &attachment : graph.organAttachments()) {
		hash_string(&value, attachment.identifier());
		hash_string(&value, attachment.hostNodeIdentifier());
		hash_string(&value, attachment.shapeIdentifier());
		hash_string(&value, attachment.hostInterfaceIdentifier());
		hash_string(&value, attachment.organInterfaceIdentifier());
		const int type = static_cast<int>(attachment.organType());
		const float age = attachment.developmentalAge();
		hash_bytes(&value, &type, sizeof(type));
		hash_bytes(&value, &age, sizeof(age));
		hash_bytes(
			&value, &attachment.localTransform()[0][0], sizeof(glm::mat4));
	}
	for (const VegetationBud &bud : graph.buds()) {
		hash_string(&value, bud.identifier());
		hash_string(&value, bud.hostNodeIdentifier());
		hash_bytes(&value, &bud.direction().x, sizeof(float));
		hash_bytes(&value, &bud.direction().y, sizeof(float));
		hash_bytes(&value, &bud.direction().z, sizeof(float));
		const float age = bud.developmentalAge();
		const float probability = bud.activationProbability();
		const int state = static_cast<int>(bud.state());
		const int type = static_cast<int>(bud.organType());
		hash_bytes(&value, &age, sizeof(age));
		hash_bytes(&value, &probability, sizeof(probability));
		hash_bytes(&value, &state, sizeof(state));
		hash_bytes(&value, &type, sizeof(type));
	}
	for (const VegetationGrowthTip &growth_tip : graph.growthTips()) {
		hash_string(&value, growth_tip.identifier());
		hash_string(&value, growth_tip.hostNodeIdentifier());
		hash_bytes(&value, &growth_tip.direction().x, sizeof(float));
		hash_bytes(&value, &growth_tip.direction().y, sizeof(float));
		hash_bytes(&value, &growth_tip.direction().z, sizeof(float));
		const bool active = growth_tip.isActive();
		hash_bytes(&value, &active, sizeof(active));
	}
	return value;
}
