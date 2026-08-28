#include "vehicle/parametric/service/ModernCarParametricDeterministicHashService.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace {

class ParametricModelHashAccumulator
{
public:
	void appendByte(std::uint8_t value)
	{
		hash_ ^= value;
		hash_ *= 1099511628211ULL;
	}

	template <typename Value>
	void appendValue(const Value &value)
	{
		const auto *bytes = reinterpret_cast<const std::uint8_t *>(&value);
		for (std::size_t index = 0u; index < sizeof(Value); ++index) {
			appendByte(bytes[index]);
		}
	}

	void appendString(const std::string &value)
	{
		for (unsigned char character : value) appendByte(character);
		appendByte(0xffu);
	}

	void appendVector2(const glm::dvec2 &value)
	{
		appendValue(value.x);
		appendValue(value.y);
	}

	void appendVector3(const glm::dvec3 &value)
	{
		appendValue(value.x);
		appendValue(value.y);
		appendValue(value.z);
	}

	std::uint64_t value() const { return hash_; }

private:
	std::uint64_t hash_ = 14695981039346656037ULL;
};

void append_station_function(
	ParametricModelHashAccumulator *hash,
	const StationFunction &function)
{
	hash->appendString(function.identifier());
	hash->appendValue(static_cast<int>(function.interpolationKind()));
	for (const StationFunctionKnot &knot : function.knots()) {
		hash->appendValue(knot.position());
		hash->appendValue(knot.value());
	}
}

void append_variant(
	ParametricModelHashAccumulator *hash,
	const ModernCarVariantDefinition &variant)
{
	hash->appendString(variant.identifier());
	hash->appendString(variant.displayName());
	hash->appendString(variant.description());
	hash->appendString(variant.packageBasis());
	const VehiclePackageParameters &package = variant.package();
	hash->appendString(package.identifier());
	hash->appendValue(package.length());
	hash->appendValue(package.width());
	hash->appendValue(package.height());
	hash->appendValue(package.wheelbase());
	hash->appendValue(package.groundClearance());
	hash->appendValue(package.frontOverhang());
	hash->appendValue(package.rearOverhang());
	const VehicleWheelParameters &wheels = variant.wheels();
	hash->appendValue(wheels.radius());
	hash->appendValue(wheels.width());
	hash->appendValue(wheels.frontTrack());
	hash->appendValue(wheels.rearTrack());
	const VehicleStyleParameters &style = variant.style();
	hash->appendValue(style.roofScale());
	hash->appendValue(style.bodyFlare());
	hash->appendValue(style.cabinFrontFactor());
	hash->appendValue(style.cabinRearFactor());
	hash->appendValue(style.tailTaper());
	hash->appendValue(style.noseTaper());
	hash->appendValue(style.spoilerScale());
	hash->appendValue(style.splitterScale());
	hash->appendValue(style.grilleScale());
	hash->appendValue(style.hasCrossoverCladding());
	const VehiclePowertrainIntent &powertrain = variant.powertrain();
	hash->appendValue(static_cast<int>(powertrain.powerSource()));
	hash->appendValue(static_cast<int>(powertrain.driveIntent()));
	hash->appendValue(powertrain.drivesFrontAxle());
	hash->appendValue(powertrain.drivesRearAxle());
	hash->appendString(powertrain.energyStorageEnvelope().identifier());
	hash->appendVector3(glm::dvec3(powertrain.energyStorageEnvelope().minimum()));
	hash->appendVector3(glm::dvec3(powertrain.energyStorageEnvelope().maximum()));
	hash->appendValue(powertrain.exhaustOutletCount());

	const ParametricBodyDefinition &body = variant.body();
	const VehicleLongitudinalDomain &domain = body.longitudinalDomain();
	hash->appendValue(domain.rear());
	hash->appendValue(domain.middle());
	hash->appendValue(domain.front());
	hash->appendValue(domain.cabinRear());
	hash->appendValue(domain.cabinMiddle());
	hash->appendValue(domain.cabinFront());
	const LowerBodyFieldDefinition &lower = body.lowerBody();
	hash->appendValue(lower.longitudinalExponent());
	hash->appendValue(lower.lateralExponent());
	hash->appendValue(lower.verticalExponent());
	hash->appendValue(lower.widthScale());
	append_station_function(hash, lower.widthRatioByStation());
	append_station_function(hash, lower.centreHeightByStation());
	append_station_function(hash, lower.halfHeightByStation());
	const GreenhouseFieldDefinition &greenhouse = body.greenhouse();
	hash->appendValue(greenhouse.longitudinalExponent());
	hash->appendValue(greenhouse.lateralExponent());
	hash->appendValue(greenhouse.verticalExponent());
	hash->appendValue(greenhouse.widthScale());
	hash->appendValue(greenhouse.roofCrownLongitudinalExponent());
	hash->appendValue(greenhouse.roofCrownVerticalExponent());
	append_station_function(hash, greenhouse.widthRatioByStation());
	append_station_function(hash, greenhouse.centreHeightByStation());
	append_station_function(hash, greenhouse.halfHeightByStation());
	const FenderFieldDefinition &fenders = body.fenders();
	hash->appendValue(fenders.frontLongitudinalRadius());
	hash->appendValue(fenders.rearLongitudinalRadius());
	hash->appendValue(fenders.frontVerticalRadius());
	hash->appendValue(fenders.rearVerticalRadius());
	hash->appendValue(fenders.centreHeight());
	hash->appendValue(fenders.halfWidth());
	hash->appendValue(fenders.longitudinalExponent());
	hash->appendValue(fenders.lateralExponent());
	hash->appendValue(fenders.verticalExponent());
	hash->appendValue(body.wheelhouseDifference().clearance());
	hash->appendValue(body.blend().lowerGreenhouseSmoothness());
	hash->appendValue(body.blend().fenderSmoothness());
	for (const ParametricCharacterCurve &curve : body.characterCurveNetwork().curves()) {
		hash->appendString(curve.identifier());
	}
	hash->appendValue(body.characterCurveNetwork().samplesPerCurve());
	hash->appendValue(body.bodySectionNetwork().sectionCount());
	for (int colour : variant.exteriorFeatures().bodyRgb()) hash->appendValue(colour);
	hash->appendValue(variant.exteriorFeatures().exhaustOutletCount());
}

} // namespace

std::uint64_t ModernCarParametricDeterministicHashService::calculateFamily(
	const ModernCarFamilyDefinition &family) const
{
	ParametricModelHashAccumulator hash;
	hash.appendString("ProGen3D-MCP_OMv1-Family-v1");
	hash.appendString(family.identifier());
	for (const ModernCarVariantDefinition &variant : family.variants()) {
		append_variant(&hash, variant);
	}
	return hash.value();
}

std::uint64_t ModernCarParametricDeterministicHashService::calculateVariant(
	const ModernCarVariantDefinition &variant) const
{
	ParametricModelHashAccumulator hash;
	hash.appendString("ProGen3D-MCP_OMv1-Variant-v1");
	append_variant(&hash, variant);
	return hash.value();
}

std::uint64_t ModernCarParametricDeterministicHashService::calculateGenerationPolicy(
	const ParametricModelGenerationPolicy &policy) const
{
	ParametricModelHashAccumulator hash;
	hash.appendString("ProGen3D-MCP_OMv1-GenerationPolicy-v1");
	hash.appendString(policy.identifier());
	hash.appendValue(static_cast<int>(policy.resolution()));
	hash.appendValue(policy.longitudinalSamples());
	hash.appendValue(policy.lateralSamples());
	hash.appendValue(policy.verticalSamples());
	hash.appendValue(policy.isoValue());
	hash.appendValue(policy.allowsStationExtrapolation());
	return hash.value();
}

std::uint64_t ModernCarParametricDeterministicHashService::calculateSourceManifest(
	const ParametricModelSourceManifest &source_manifest) const
{
	ParametricModelHashAccumulator hash;
	hash.appendString("ProGen3D-MCP_OMv1-SourceManifest-v1");
	hash.appendString(source_manifest.schema());
	hash.appendString(source_manifest.generatorIdentifier());
	hash.appendString(source_manifest.pythonVersion());
	for (const ParametricSourceFileRecord &file : source_manifest.files()) {
		hash.appendString(file.path());
		hash.appendString(file.sha256());
	}
	hash.appendString(source_manifest.deterministicHash());
	return hash.value();
}

std::uint64_t ModernCarParametricDeterministicHashService::calculateBodyMesh(
	const GeneratedBodyMesh &body_mesh) const
{
	ParametricModelHashAccumulator hash;
	hash.appendString("ProGen3D-MCP_OMv1-BodyMesh-v1");
	if (!body_mesh.mesh()) return hash.value();
	for (const glm::vec3 &vertex : body_mesh.mesh()->vertices) {
		hash.appendVector3(glm::dvec3(vertex));
	}
	for (const glm::ivec3 &face : body_mesh.mesh()->faces) {
		hash.appendValue(face.x);
		hash.appendValue(face.y);
		hash.appendValue(face.z);
	}
	return hash.value();
}

std::uint64_t ModernCarParametricDeterministicHashService::calculateRealization(
	const GeneratedVehicleRealization &realization) const
{
	ParametricModelHashAccumulator hash;
	hash.appendString("ProGen3D-MCP_OMv1-GeneratedVehicleRealization-v1");
	hash.appendString(realization.identifier());
	hash.appendString(realization.variantIdentifier());
	hash.appendValue(realization.sourceManifestHash());
	hash.appendValue(realization.generationPolicyHash());
	hash.appendValue(calculateBodyMesh(realization.bodyMesh()));
	for (const GeneratedCharacterCurve &curve : realization.characterCurves().curves()) {
		hash.appendString(curve.identifier());
		for (const glm::dvec3 &point : curve.points()) hash.appendVector3(point);
	}
	for (const GeneratedBodySection &section : realization.bodySections().sections()) {
		hash.appendString(section.identifier());
		hash.appendValue(section.station());
		for (const glm::dvec3 &point : section.points()) hash.appendVector3(point);
	}
	for (const GeneratedVehicleObservation &observation :
	     realization.observations().observations()) {
		hash.appendString(observation.identifier());
		hash.appendValue(static_cast<int>(observation.view()));
		hash.appendVector3(observation.direction());
		hash.appendVector3(observation.up());
		for (const glm::dvec2 &point : observation.normalizedSilhouette()) {
			hash.appendVector2(point);
		}
	}
	for (const GeneratedArtifactReference &artifact :
	     realization.artifactManifest().artifacts()) {
		hash.appendString(artifact.semanticRole());
		hash.appendString(artifact.path());
		hash.appendString(artifact.sha256());
	}
	return hash.value();
}
