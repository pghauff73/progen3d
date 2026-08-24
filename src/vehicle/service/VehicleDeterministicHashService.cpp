#include "vehicle/service/VehicleDeterministicHashService.h"

#include "geometry/service/MeshTopologyAnalyzer.h"

#include <cstdint>
#include <cstring>
#include <string>

namespace {

class VehicleHashAccumulator
{
public:
	void appendByte(std::uint8_t value)
	{
		hash_ ^= value;
		hash_ *= 1099511628211ULL;
	}

	void appendString(const std::string &value)
	{
		for (unsigned char character : value) appendByte(character);
		appendByte(0xffu);
	}

	template <typename Value>
	void appendValue(const Value &value)
	{
		const auto *bytes = reinterpret_cast<const std::uint8_t *>(&value);
		for (std::size_t index = 0u; index < sizeof(Value); ++index) {
			appendByte(bytes[index]);
		}
	}

	std::uint64_t value() const { return hash_; }

private:
	std::uint64_t hash_ = 14695981039346656037ULL;
};

} // namespace

std::uint64_t VehicleDeterministicHashService::calculate(
	const VehicleDefinition &definition,
	const std::vector<VehiclePlacedAssembly> &assemblies,
	const std::vector<VehicleJoint> &joints,
	const GeneratedPrimitiveMesh &combined_mesh) const
{
	VehicleHashAccumulator hash;
	hash.appendString("MVGv1-ModernVehicleAssembly-v1");
	hash.appendString(definition.identifier());
	hash.appendValue(definition.package().overallLength());
	hash.appendValue(definition.package().overallWidth());
	hash.appendValue(definition.package().overallHeight());
	hash.appendValue(definition.package().wheelbase());
	for (const VehiclePlacedAssembly &assembly : assemblies) {
		hash.appendString(assembly.objectIdentifier());
		for (const VehicleAssemblyPart &part : assembly.geometry().parts()) {
			hash.appendString(part.partIdentifier());
			hash.appendString(part.semanticRole());
			if (part.shape()) {
				hash.appendString(part.shape()->key().canonicalValue());
			}
		}
		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) {
				hash.appendValue(assembly.localTransform()[column][row]);
			}
		}
	}
	for (const VehicleJoint &joint : joints) {
		hash.appendString(joint.jointIdentifier());
		hash.appendValue(static_cast<int>(joint.type()));
		hash.appendValue(joint.minimumState());
		hash.appendValue(joint.maximumState());
		hash.appendValue(joint.currentState());
	}
	if (combined_mesh.mesh()) {
		const MeshTopologyReport topology = MeshTopologyAnalyzer().analyze(
			*combined_mesh.mesh(), combined_mesh.faceSurfaceTags());
		hash.appendValue(topology.topology_hash);
	}
	return hash.value();
}
