#include "vehicle/service/VehicleMvp26DeterministicHashService.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace {

class VehicleMvp26HashAccumulator
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

} // namespace

std::uint64_t VehicleMvp26DeterministicHashService::calculate(
	const VehicleMvp26Architecture &architecture) const
{
	VehicleMvp26HashAccumulator hash;
	hash.appendString("MVP2.6-ParametricEvidenceIntegration-v1");
	hash.appendString(architecture.identifier());
	hash.appendValue(architecture.acceptedMvp25Hash());
	hash.appendString(architecture.variant().identifier());
	hash.appendString(architecture.variant().displayName());
	const VehiclePackageEvidence &package = architecture.variant().packageCandidate();
	hash.appendValue(package.overallLength());
	hash.appendValue(package.overallWidth());
	hash.appendValue(package.overallHeight());
	hash.appendValue(package.wheelbase());
	hash.appendValue(package.frontTrack());
	hash.appendValue(package.rearTrack());
	hash.appendValue(package.frontOverhang());
	hash.appendValue(package.rearOverhang());
	const VehicleParametricShapePrior &shape = architecture.parametricShapePrior();
	hash.appendString(shape.identifier());
	hash.appendString(shape.sourceVariantIdentifier());
	for (const GeneratedCharacterCurve &curve : shape.characterCurves().curves()) {
		hash.appendString(curve.identifier());
		for (const glm::dvec3 &point : curve.points()) hash.appendVector3(point);
	}
	for (const GeneratedBodySection &section : shape.bodySections().sections()) {
		hash.appendString(section.identifier());
		hash.appendValue(section.station());
		for (const glm::dvec3 &point : section.points()) hash.appendVector3(point);
	}
	for (const VehicleParametricStyleTerm &term : shape.styleTerms()) {
		hash.appendString(term.identifier());
		hash.appendValue(term.value());
		hash.appendValue(term.weight());
		hash.appendValue(static_cast<int>(term.strength()));
	}
	const VehicleParametricSourceEvidence &source = architecture.sourceEvidence();
	hash.appendString(source.identifier());
	hash.appendString(source.coordinateFrameIdentifier());
	hash.appendValue(source.sourceManifestHash());
	hash.appendValue(source.generationPolicyHash());
	hash.appendValue(source.generatedGeometryHash());
	for (const GeneratedVehicleObservation &observation :
	     source.observations().observations()) {
		hash.appendString(observation.identifier());
		hash.appendValue(static_cast<int>(observation.view()));
		for (const glm::dvec2 &point : observation.normalizedSilhouette()) {
			hash.appendVector2(point);
		}
	}
	const VehicleParametricFitReport &fit = architecture.fitReport();
	hash.appendString(fit.identifier());
	hash.appendValue(fit.maximumPackageResidual());
	hash.appendValue(fit.packageAuthorityPreserved());
	for (const VehicleParametricViewFit &view : fit.viewFits()) {
		hash.appendValue(static_cast<int>(view.view()));
		hash.appendValue(view.silhouetteIntersectionOverUnion());
		hash.appendValue(view.requiredIntersectionOverUnion());
		hash.appendVector2(view.sourceToAuthorityScale());
		hash.appendValue(view.sourceToAuthorityLongitudinalHeightShear());
		hash.appendVector2(view.sourceToAuthorityTranslation());
	}
	for (const std::string &diagnostic : fit.blockingDiagnostics()) {
		hash.appendString(diagnostic);
	}
	return hash.value();
}
