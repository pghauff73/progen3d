#include "RenderQueue.h"

#include "ManifestInput.h"
#include "RenderFiles.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <utility>

namespace {

namespace fs = std::filesystem;
using render_files::FileLease;
using render_files::LockWait;
using namespace std::chrono_literals;

constexpr const char *queue_lock = ".queue.lock";
constexpr const char *worker_lock = ".worker.lock";
constexpr const char *job_name = "job.json";
constexpr const char *grammar_name = "grammar.snapshot";
constexpr const char *status_name = "status.json";
constexpr std::size_t id_width = 12;

class GpuWorkerSlot {
public:
	GpuWorkerSlot()
	{
		mutex_ = CreateMutexW(nullptr, FALSE, L"Global\\ProGen3D.RenderWorker");
		if (mutex_ == nullptr) {
			throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
			                        "Cannot open the Windows GPU worker slot");
		}
		const DWORD result = WaitForSingleObject(mutex_, 0);
		if (result == WAIT_OBJECT_0 || result == WAIT_ABANDONED) return;
		const DWORD error = GetLastError();
		CloseHandle(mutex_);
		if (result == WAIT_TIMEOUT) throw std::runtime_error("A ProGen3D GPU worker is already running on this PC.");
		throw std::system_error(static_cast<int>(error), std::system_category(), "Cannot acquire the Windows GPU worker slot");
	}

	~GpuWorkerSlot()
	{
		ReleaseMutex(mutex_);
		CloseHandle(mutex_);
	}

	GpuWorkerSlot(const GpuWorkerSlot &) = delete;
	GpuWorkerSlot &operator=(const GpuWorkerSlot &) = delete;

private:
	HANDLE mutex_ = nullptr;
};

bool is_job_id(std::string_view id)
{
	return id.size() == id_width &&
	       std::all_of(id.begin(), id.end(), [](char ch) { return ch >= '0' && ch <= '9'; });
}

std::string next_id(const std::vector<std::string> &ids)
{
	const auto number = ids.empty() ? 1ULL : std::stoull(ids.back()) + 1;
	std::ostringstream output;
	output << std::setfill('0') << std::setw(static_cast<int>(id_width)) << number;
	if (!is_job_id(output.str())) throw std::runtime_error("The render queue ran out of job IDs.");
	return output.str();
}

Json::Int64 unix_milliseconds()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}

Json::Value failed_render(const std::string &message)
{
	Json::Value result;
	result["schema_version"] = 1;
	result["command"] = "render";
	result["ok"] = false;
	result["exit_code"] = 5;
	result["error"] = message;
	return result;
}

void set_progress(Json::Value &job, const RenderProgress &progress)
{
	job["completed_frames"] = progress.completedFrames();
	job["rendered_frames"] = progress.rendered_frames;
	job["reused_frames"] = progress.reused_frames;
}

}

RenderQueue::RenderQueue(const fs::path &directory)
{
	fs::create_directories(directory / "jobs");
	directory_ = fs::canonical(directory);
}

fs::path RenderQueue::jobDirectory(const std::string &id) const
{
	if (!is_job_id(id)) throw std::runtime_error("Expected a 12-digit render job ID.");
	return directory_ / "jobs" / id;
}

std::vector<std::string> RenderQueue::jobIds() const
{
	std::vector<std::string> ids;
	for (const auto &entry : fs::directory_iterator(directory_ / "jobs")) {
		auto id = entry.path().filename().u8string();
		if (is_job_id(id) && fs::is_directory(entry.symlink_status())) ids.push_back(std::move(id));
	}
	std::sort(ids.begin(), ids.end());
	return ids;
}

Json::Value RenderQueue::loadJob(const std::string &id) const
{
	const auto record = jobDirectory(id) / status_name;
	if (!fs::is_regular_file(record)) throw std::runtime_error("No such render job: " + id);
	auto job = manifest_input::read_json(record);
	if (!job.isObject() || job["schema_version"] != 1 || job["id"] != id || !job["state"].isString()) {
		throw std::runtime_error("Invalid status record for render job " + id + ".");
	}
	return job;
}

void RenderQueue::saveJob(Json::Value &job) const
{
	job["updated_unix_ms"] = unix_milliseconds();
	render_files::write_json(jobDirectory(job["id"].asString()) / status_name, job);
}

Json::Value RenderQueue::enqueue(const fs::path &source_job, const Json::Value &job,
                                std::string_view grammar, std::uint32_t total_frames) const
{
	const FileLease lock(directory_ / queue_lock);
	const auto id = next_id(jobIds());
	const auto destination = jobDirectory(id);
	const auto staging = destination.parent_path() / (L".submission-" + render_files::unique_suffix() + L".tmp");
	if (!fs::create_directory(staging)) throw std::runtime_error("Cannot stage a render queue submission.");
	try {
		render_files::write_json(staging / job_name, job);
		render_files::write_text(staging / grammar_name, grammar);

		Json::Value record;
		record["schema_version"] = 1;
		record["id"] = id;
		record["state"] = "queued";
		record["source_job"] = source_job.u8string();
		record["job_manifest"] = (destination / job_name).u8string();
		record["output_directory"] = (destination / "render").u8string();
		record["job_sha256"] = render_files::sha256_file(staging / job_name);
		record["grammar_sha256"] = render_files::sha256_text(grammar);
		record["submitted_unix_ms"] = unix_milliseconds();
		record["updated_unix_ms"] = record["submitted_unix_ms"];
		record["attempt"] = 0;
		record["total_frames"] = total_frames;
		set_progress(record, {});
		render_files::write_json(staging / status_name, record);
		render_files::move_into_place(staging, destination);
		return record;
	} catch (...) {
		std::error_code ignored;
		for (const char *name : {job_name, grammar_name, status_name}) fs::remove(staging / name, ignored);
		fs::remove(staging, ignored);
		throw;
	}
}

bool RenderQueue::workerActive() const
{
	return !FileLease(directory_ / worker_lock, LockWait::try_once).acquired();
}

void RenderQueue::recoverInterruptedJobs() const
{
	for (const auto &id : jobIds()) {
		auto job = loadJob(id);
		if (job["state"] != "running" && job["state"] != "cancelling") continue;
		job["state"] = job["state"] == "cancelling" ? "cancelled" : "interrupted";
		job.removeMember("worker_pid");
		render_files::discard_abandoned_writes(jobDirectory(id) / status_name);
		saveJob(job);
	}
}

Json::Value RenderQueue::status(const std::optional<std::string> &id) const
{
	const FileLease lock(directory_ / queue_lock);
	const bool worker_active = workerActive();
	if (!worker_active) recoverInterruptedJobs();
	Json::Value report;
	report["queue_directory"] = directory_.u8string();
	report["worker_running"] = worker_active;
	if (id) {
		report["job"] = loadJob(*id);
	} else {
		report["jobs"] = Json::Value(Json::arrayValue);
		for (const auto &entry : jobIds()) report["jobs"].append(loadJob(entry));
	}
	return report;
}

Json::Value RenderQueue::cancel(const std::string &id) const
{
	const FileLease lock(directory_ / queue_lock);
	if (!workerActive()) recoverInterruptedJobs();
	auto job = loadJob(id);
	if (job["state"] == "running") {
		job["state"] = "cancelling";
		saveJob(job);
	} else if (job["state"] == "queued" || job["state"] == "interrupted") {
		job["state"] = "cancelled";
		saveJob(job);
	}
	return job;
}

Json::Value RenderQueue::resume(const std::string &id) const
{
	const FileLease lock(directory_ / queue_lock);
	if (!workerActive()) recoverInterruptedJobs();
	auto job = loadJob(id);
	if (job["state"] == "queued") return job;
	if (job["state"] != "cancelled" && job["state"] != "interrupted" && job["state"] != "failed") {
		throw std::runtime_error("Only cancelled, interrupted, or failed jobs can be resumed.");
	}
	job["state"] = "queued";
	job.removeMember("result");
	saveJob(job);
	return job;
}

std::optional<std::string> RenderQueue::claimNextJob() const
{
	const FileLease lock(directory_ / queue_lock);
	for (auto id : jobIds()) {
		auto job = loadJob(id);
		if (job["state"] != "queued") continue;
		job["state"] = "running";
		job["worker_pid"] = static_cast<Json::UInt>(GetCurrentProcessId());
		job["attempt"] = job["attempt"].asUInt64() + 1;
		set_progress(job, {});
		saveJob(job);
		return id;
	}
	return std::nullopt;
}

void RenderQueue::saveProgress(const std::string &id, const RenderProgress &progress) const
{
	const FileLease lock(directory_ / queue_lock);
	auto job = loadJob(id);
	set_progress(job, progress);
	saveJob(job);
}

Json::Value RenderQueue::renderClaim(const std::string &id, const RenderJob &render) const
{
	try {
		const auto directory = jobDirectory(id);
		Json::Value input;
		{
			const FileLease lock(directory_ / queue_lock);
			input = loadJob(id);
		}
		const auto manifest = directory / job_name;
		const auto grammar = directory / grammar_name;
		if (render_files::sha256_file(manifest) != input["job_sha256"].asString() ||
		    render_files::sha256_file(grammar) != input["grammar_sha256"].asString()) {
			throw std::runtime_error("Queued render inputs failed their SHA-256 checks.");
		}

		RenderExecution execution;
		execution.checkpoint_directory = directory / "render";
		execution.grammar_snapshot = grammar;
		execution.resume = true;
		auto next_check = std::chrono::steady_clock::time_point::min();
		bool cancelled = false;
		execution.cancelled = [&] {
			const auto now = std::chrono::steady_clock::now();
			if (!cancelled && now >= next_check) {
				const FileLease lock(directory_ / queue_lock);
				cancelled = loadJob(id)["state"] == "cancelling";
				next_check = now + 100ms;
			}
			return cancelled;
		};
		execution.publish_progress = [&](const RenderProgress &progress) {
			saveProgress(id, progress);
			next_check = std::chrono::steady_clock::time_point::min();
		};
		return render(manifest, execution);
	} catch (const std::exception &error) {
		return failed_render(error.what());
	}
}

Json::Value RenderQueue::finishJob(const std::string &id, Json::Value result) const
{
	const FileLease lock(directory_ / queue_lock);
	auto job = loadJob(id);
	if (result["ok"].asBool()) {
		job["state"] = "succeeded";
	} else if (result["exit_code"].asInt() == render_cancelled_exit_code) {
		job["state"] = "cancelled";
	} else {
		job["state"] = "failed";
	}
	job.removeMember("worker_pid");
	job["result"] = std::move(result);
	saveJob(job);
	return job;
}

void RenderQueue::work(bool once, const ProbeGraphics &probe, const RenderJob &render, Json::Value &report) const
{
	const GpuWorkerSlot slot;
	const FileLease lease(directory_ / worker_lock, LockWait::try_once);
	if (!lease.acquired()) throw std::runtime_error("This queue already has a GPU worker.");
	{
		const FileLease lock(directory_ / queue_lock);
		recoverInterruptedJobs();
	}

	probe(report);
	report["queue_directory"] = directory_.u8string();
	report["processed_jobs"] = 0;
	while (true) {
		if (const auto id = claimNextJob()) {
			report["job"] = finishJob(*id, renderClaim(*id, render));
			report["processed_jobs"] = report["processed_jobs"].asUInt64() + 1;
		} else if (!once) {
			std::this_thread::sleep_for(250ms);
		}
		if (once) return;
	}
}
