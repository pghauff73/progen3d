#include "editor/service/GrammarCompilationService.h"

#include "Context.h"
#include "GrammarRuntime.h"
#include "grammar.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>
#include <utility>

extern void errorout(std::string error_str);

namespace {

bool has_power_taxonomy(const SpatialBuildingObject &object, const char *role)
{
	const auto &segments = object.identity().taxonomyPath().segments();
	return segments.size() >= 2 && segments[0] == "Power" && segments[1] == role;
}

bool is_electric_motor(const SpatialBuildingObject &object)
{
	const auto &object_class = object.identity().objectClass().value();
	return object_class == "ElectricMotor" ||
	       object_class == "PermanentMagnetMotor" ||
	       object_class == "PermanentMagnetMotorStator" ||
	       object_class == "BrushedDCMotor" ||
	       object_class == "BrushlessDCMotor" ||
	       object_class == "InductionMotor" ||
	       object_class == "LinearElectricMotor" ||
	       object_class == "ServoMotor" ||
	       object_class == "StepperMotor" ||
	       object_class == "SynchronousMotor" ||
	       has_power_taxonomy(object, "ElectricMotor");
}

bool transfers_electrical_power(const SpatialBuildingObject &object)
{
	const auto &object_class = object.identity().objectClass().value();
	if (!has_power_taxonomy(object, "Transfer") || is_electric_motor(object) ||
	    object_class == "OpenSwitch" || object_class == "OpenElectricalSwitch" ||
	    object_class == "PowerSourceCatalogEntry") {
		return false;
	}
	const auto &segments = object.identity().taxonomyPath().segments();
	return std::find(segments.begin() + 2, segments.end(), "Open") == segments.end();
}

bool is_electrical_endpoint(const SpatialBuildingObject *object,
                            const SpatialInterfaceReference &endpoint)
{
	if (object == nullptr) return false;
	const auto *port = object->findInterface(endpoint.interfaceId());
	return port != nullptr && port->type() == SpatialInterfaceType::ElectricalPort;
}

void warn_about_unpowered_motors(const SpatialBuildingModel &model)
{
	std::queue<SpatialObjectId> ready;
	std::set<SpatialObjectId> powered;
	const auto &objects = model.objects();
	for (const auto &object : objects.objects()) {
		const auto &identity = object.identity();
		if (identity.objectClass().value() == "PowerSource" && !is_electric_motor(object)) {
			powered.insert(identity.objectId());
			ready.push(identity.objectId());
		}
	}

	std::map<SpatialObjectId, std::vector<const SpatialBuildingObject *>> outgoing;
	for (const auto &connection : model.connectionGraph().connections()) {
		if (connection.connectionType() != SpatialConnectionType::Powers ||
		    connection.state() == SpatialConnectionState::Broken ||
		    connection.state() == SpatialConnectionState::Invalid) {
			continue;
		}
		const auto &source = connection.sourceInterface();
		const auto &target = connection.targetInterface();
		const auto *target_object = objects.find(target.objectId());
		if (is_electrical_endpoint(objects.find(source.objectId()), source) &&
		    is_electrical_endpoint(target_object, target)) {
			outgoing[source.objectId()].push_back(target_object);
		}
	}

	while (!ready.empty()) {
		const auto connections = outgoing.find(ready.front());
		ready.pop();
		if (connections == outgoing.end()) continue;
		for (const auto *target : connections->second) {
			const auto &id = target->identity().objectId();
			if (powered.insert(id).second && transfers_electrical_power(*target)) ready.push(id);
		}
	}

	for (const auto &object : objects.objects()) {
		const auto &id = object.identity().objectId();
		if (is_electric_motor(object) && powered.count(id) == 0) {
			errorout("Grammar execution warning: MOTOR_WITHOUT_POWER_SOURCE: Electric motor '" +
			         id.value() + "' has no directed electrical power path from a PowerSource.");
		}
	}
}

} // namespace

GrammarCompilationService::GrammarCompilationService(
	RandomSeedFunction random_seed_function,
	StructuralValidationFunction structural_validation_function)
	: random_seed_function_(std::move(random_seed_function)),
	  structural_validation_function_(std::move(structural_validation_function))
{
}

std::unique_ptr<SceneGenerationResult> GrammarCompilationService::compile(
	const SceneGenerationRequest &request) const
{
	const auto generation_started_at = std::chrono::steady_clock::now();
	auto result = std::make_unique<SceneGenerationResult>();
	result->request_id = request.request_id;
	result->design_nonce = request.design_nonce;
	result->evaluation_time = request.evaluation_time;
	result->time_sample = request.time_sample;

	try {
		if (random_seed_function_) {
			random_seed_function_(request.source_text, request.design_nonce);
		}

		auto grammar_document = std::make_unique<GrammarDocument>();
		if (!setGrammarEvaluationTime(grammar_document.get(), request.evaluation_time)) {
			throw std::runtime_error("grammar evaluation time is not finite or representable");
		}
		setGrammarEvaluationDiagnosticsQuiet(grammar_document.get(), request.time_sample);
		grammar_document->loadFromSourceText(request.source_text, request.source_name);

		const bool parser_succeeded = grammar_document->error_details.empty();
		const bool structure_succeeded =
			parser_succeeded &&
			(!structural_validation_function_ ||
			 structural_validation_function_(*grammar_document, request.time_sample));
		if (structure_succeeded) {
			grammar_document->addContext();
			grammar_document->context->genPrimitives();
			grammar_document->generateGeometry();
		}

		result->diagnostic_lines = grammar_document->error_lines;
		result->diagnostics = grammar_document->error_details;
		if (structure_succeeded && result->diagnostics.empty() &&
		    grammar_document->context != nullptr) {
			// Authoring compiles check topology. Frame rebuilds would repeat editor warnings.
			if (!request.time_sample) {
				if (const auto &model = grammar_document->context->spatialBuildingModel()) {
					warn_about_unpowered_motors(*model);
				}
			}
			result->scene_snapshot = std::make_unique<GeneratedSceneSnapshot>(
				std::move(grammar_document), request.design_nonce, request.evaluation_time);
		} else if (result->diagnostics.empty()) {
			result->error = "Scene generation did not produce a complete scene context.";
		}
	}
	catch (const std::exception &exception) {
		result->error = std::string("Regeneration failed: ") + exception.what();
	}
	catch (...) {
		result->error = "Regeneration failed.";
	}

	result->generation_milliseconds =
		std::chrono::duration<double, std::milli>(
			std::chrono::steady_clock::now() - generation_started_at)
			.count();
	return result;
}
