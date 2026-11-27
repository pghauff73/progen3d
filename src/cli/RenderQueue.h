#pragma once

#include "RenderExecution.h"

#include <json/json.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

class RenderQueue {
public:
	using ProbeGraphics = std::function<void(Json::Value &)>;
	using RenderJob = std::function<Json::Value(const std::filesystem::path &, const RenderExecution &)>;

	explicit RenderQueue(const std::filesystem::path &directory);

	Json::Value enqueue(const std::filesystem::path &source_job, const Json::Value &job,
	                    std::string_view grammar, std::uint32_t total_frames) const;
	Json::Value status(const std::optional<std::string> &id) const;
	Json::Value cancel(const std::string &id) const;
	Json::Value resume(const std::string &id) const;
	void work(bool once, const ProbeGraphics &probe, const RenderJob &render, Json::Value &report) const;

	const std::filesystem::path &directory() const { return directory_; }

private:
	std::filesystem::path jobDirectory(const std::string &id) const;
	std::vector<std::string> jobIds() const;
	Json::Value loadJob(const std::string &id) const;
	void saveJob(Json::Value &job) const;

	bool workerActive() const;
	void recoverInterruptedJobs() const;
	std::optional<std::string> claimNextJob() const;
	void saveProgress(const std::string &id, const RenderProgress &progress) const;
	Json::Value renderClaim(const std::string &id, const RenderJob &render) const;
	Json::Value finishJob(const std::string &id, Json::Value result) const;

	std::filesystem::path directory_;
};
