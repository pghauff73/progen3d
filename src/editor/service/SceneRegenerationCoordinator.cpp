#include "editor/service/SceneRegenerationCoordinator.h"

#include <exception>
#include <utility>

SceneRegenerationCoordinator::SceneRegenerationCoordinator(GenerationFunction generation_function)
	: generation_function_(std::move(generation_function))
{
}

SceneRegenerationCoordinator::~SceneRegenerationCoordinator()
{
	shutdown();
}

std::uint64_t SceneRegenerationCoordinator::requestGeneration(std::string source_text,
                                                             double evaluation_time,
                                                             std::uint64_t design_nonce,
                                                             bool time_sample)
{
	if (time_sample && scheduled_request_.has_value()) {
		return latest_requested_id_;
	}
	cancelScheduledRequest();
	SceneGenerationRequest request =
		createRequest(std::move(source_text), evaluation_time, design_nonce, time_sample);
	submitRequest(std::move(request));
	return latest_requested_id_;
}

std::uint64_t SceneRegenerationCoordinator::scheduleGeneration(
	std::string source_text,
	double evaluation_time,
	std::uint64_t design_nonce,
	bool time_sample,
	double scheduled_start_time)
{
	scheduled_request_ =
		createRequest(std::move(source_text), evaluation_time, design_nonce, time_sample);
	scheduled_start_time_ = scheduled_start_time;
	return latest_requested_id_;
}

SceneGenerationRequest SceneRegenerationCoordinator::createRequest(
	std::string source_text,
	double evaluation_time,
	std::uint64_t design_nonce,
	bool time_sample)
{
	SceneGenerationRequest request;
	request.request_id = ++next_request_id_;
	request.source_text = std::move(source_text);
	request.design_nonce = design_nonce;
	request.evaluation_time = evaluation_time;
	request.time_sample = time_sample;
	latest_requested_id_ = request.request_id;
	return request;
}

void SceneRegenerationCoordinator::submitRequest(SceneGenerationRequest request)
{
	if (generation_in_progress_.load()) {
		pending_request_ = std::move(request);
		return;
	}

	startWorker(std::move(request));
}

std::unique_ptr<SceneGenerationResult> SceneRegenerationCoordinator::takeCompletedResult()
{
	std::unique_ptr<SceneGenerationResult> result;
	{
		std::lock_guard<std::mutex> lock(completed_result_mutex_);
		result = std::move(completed_result_);
	}
	if (result != nullptr) {
		joinWorkerIfIdle();
	}
	return result;
}

void SceneRegenerationCoordinator::startPendingRequest()
{
	if (generation_in_progress_.load() || !pending_request_.has_value()) {
		return;
	}
	SceneGenerationRequest request = std::move(*pending_request_);
	pending_request_.reset();
	startWorker(std::move(request));
}

bool SceneRegenerationCoordinator::startScheduledRequestIfDue(double current_time)
{
	if (!scheduled_request_.has_value() || current_time < scheduled_start_time_) {
		return false;
	}
	SceneGenerationRequest request = std::move(*scheduled_request_);
	scheduled_request_.reset();
	scheduled_start_time_ = 0.0;
	submitRequest(std::move(request));
	return true;
}

void SceneRegenerationCoordinator::cancelScheduledRequest()
{
	scheduled_request_.reset();
	scheduled_start_time_ = 0.0;
}

void SceneRegenerationCoordinator::joinWorkerIfIdle()
{
	if (generation_worker_.joinable() && !generation_in_progress_.load()) {
		generation_worker_.join();
	}
}

void SceneRegenerationCoordinator::shutdown()
{
	pending_request_.reset();
	cancelScheduledRequest();
	if (generation_worker_.joinable()) {
		generation_worker_.join();
	}
	generation_in_progress_ = false;
	inflight_design_nonce_ = 0;
	std::lock_guard<std::mutex> lock(completed_result_mutex_);
	completed_result_.reset();
}

bool SceneRegenerationCoordinator::generationInProgress() const
{
	return generation_in_progress_.load();
}

bool SceneRegenerationCoordinator::hasPendingRequest() const
{
	return pending_request_.has_value();
}

bool SceneRegenerationCoordinator::hasScheduledRequest() const
{
	return scheduled_request_.has_value();
}

bool SceneRegenerationCoordinator::isLatestRequest(std::uint64_t request_id) const
{
	return request_id == latest_requested_id_;
}

double SceneRegenerationCoordinator::scheduledStartTime() const
{
	return scheduled_start_time_;
}

std::uint64_t SceneRegenerationCoordinator::pendingDesignNonce() const
{
	return pending_request_.has_value() ? pending_request_->design_nonce : 0;
}

std::uint64_t SceneRegenerationCoordinator::inflightDesignNonce() const
{
	return inflight_design_nonce_;
}

void SceneRegenerationCoordinator::startWorker(SceneGenerationRequest request)
{
	joinWorkerIfIdle();
	inflight_design_nonce_ = request.design_nonce;
	generation_in_progress_ = true;
	generation_worker_ = std::thread([this, request = std::move(request)]() mutable {
		std::unique_ptr<SceneGenerationResult> result;
		try {
			if (generation_function_) {
				result = generation_function_(request);
			}
		}
		catch (const std::exception &exception) {
			result = std::make_unique<SceneGenerationResult>();
			result->error = std::string("Scene generation failed: ") + exception.what();
		}
		catch (...) {
			result = std::make_unique<SceneGenerationResult>();
			result->error = "Scene generation failed.";
		}
		if (result == nullptr) {
			result = std::make_unique<SceneGenerationResult>();
			result->error = "Scene generation returned no result.";
		}
		result->request_id = request.request_id;
		result->design_nonce = request.design_nonce;
		result->evaluation_time = request.evaluation_time;
		result->time_sample = request.time_sample;
		{
			std::lock_guard<std::mutex> lock(completed_result_mutex_);
			completed_result_ = std::move(result);
		}
		generation_in_progress_ = false;
	});
}
