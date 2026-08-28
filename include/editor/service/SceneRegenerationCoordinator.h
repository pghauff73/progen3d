#pragma once

#include "editor/model/SceneGenerationRequest.h"
#include "editor/model/SceneGenerationResult.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

class SceneRegenerationCoordinator
{
public:
	using GenerationFunction =
		std::function<std::unique_ptr<SceneGenerationResult>(const SceneGenerationRequest &request)>;

	explicit SceneRegenerationCoordinator(GenerationFunction generation_function);
	~SceneRegenerationCoordinator();

	SceneRegenerationCoordinator(const SceneRegenerationCoordinator &) = delete;
	SceneRegenerationCoordinator &operator=(const SceneRegenerationCoordinator &) = delete;

	std::uint64_t requestGeneration(std::string source_text,
	                                double evaluation_time,
	                                std::uint64_t design_nonce,
	                                bool time_sample);
	std::uint64_t scheduleGeneration(std::string source_text,
	                                 double evaluation_time,
	                                 std::uint64_t design_nonce,
	                                 bool time_sample,
	                                 double scheduled_start_time);
	std::unique_ptr<SceneGenerationResult> takeCompletedResult();
	void startPendingRequest();
	bool startScheduledRequestIfDue(double current_time);
	void cancelScheduledRequest();
	void joinWorkerIfIdle();
	void shutdown();

	bool generationInProgress() const;
	bool hasPendingRequest() const;
	bool hasScheduledRequest() const;
	bool isLatestRequest(std::uint64_t request_id) const;
	double scheduledStartTime() const;
	std::uint64_t pendingDesignNonce() const;
	std::uint64_t inflightDesignNonce() const;

private:
	SceneGenerationRequest createRequest(std::string source_text,
	                                     double evaluation_time,
	                                     std::uint64_t design_nonce,
	                                     bool time_sample);
	void submitRequest(SceneGenerationRequest request);
	void startWorker(SceneGenerationRequest request);

	GenerationFunction generation_function_;
	std::atomic<bool> generation_in_progress_{false};
	std::mutex completed_result_mutex_;
	std::unique_ptr<SceneGenerationResult> completed_result_;
	std::thread generation_worker_;
	std::uint64_t next_request_id_ = 0;
	std::uint64_t latest_requested_id_ = 0;
	std::optional<SceneGenerationRequest> pending_request_;
	std::optional<SceneGenerationRequest> scheduled_request_;
	double scheduled_start_time_ = 0.0;
	std::uint64_t inflight_design_nonce_ = 0;
};
