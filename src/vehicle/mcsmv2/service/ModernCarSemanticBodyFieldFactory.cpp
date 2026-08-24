#include "vehicle/mcsmv2/service/ModernCarSemanticBodyFieldFactory.h"

#include "vehicle/mcsmv2/model/DifferenceField.h"
#include "vehicle/mcsmv2/model/SemanticSectionBodyField.h"
#include "vehicle/mcsmv2/model/SuperellipsoidField.h"
#include "vehicle/mcsmv2/service/VehicleWheelMotionEnvelopeConstructionService.h"

#include <memory>

std::shared_ptr<const ImplicitScalarField>
ModernCarSemanticBodyFieldFactory::createBodyField(
	const ModernCarSemanticVariant &variant) const
{
	std::shared_ptr<const ImplicitScalarField> body_field =
		std::make_shared<SemanticSectionBodyField>(variant);
	const VehicleWheelEnvelopeSet envelope_set =
		VehicleWheelMotionEnvelopeConstructionService().construct(variant);
	for (const VehicleWheelMotionEnvelope &envelope : envelope_set.envelopes()) {
		const std::shared_ptr<const ImplicitScalarField> wheel_cavity =
			std::make_shared<SuperellipsoidField>(
				envelope.sourceCenter(), envelope.sourceRadii(), 4.0);
		body_field = std::make_shared<DifferenceField>(
			std::move(body_field), wheel_cavity);
	}
	return body_field;
}
