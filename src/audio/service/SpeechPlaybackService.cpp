#include "audio/service/SpeechPlaybackService.h"
#include "audio/service/KokoroSpeechEngine.h"

#ifdef PROGEN3D_KOKORO_ENABLED
#include <windows.h>
#include <mmsystem.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <utility>

namespace {

constexpr std::size_t max_pending_speech = 16;
constexpr std::size_t max_prepared_cues = 32;
constexpr std::size_t max_cached_samples = 16 * 1024 * 1024;
using SpeechCacheKey = std::pair<int, std::string>;

struct SpeechCancellation {
	const std::atomic<std::uint64_t> &program_revision;
	const std::atomic<std::uint64_t> &playback_revision;
	std::uint64_t program;
	std::uint64_t playback;
	bool audible;

	bool requested() const
	{
		return program_revision.load() != program ||
		       (audible && playback_revision.load() != playback);
	}

};

bool playWaveform(SpeechWaveform &waveform, const SpeechCancellation &cancellation)
{
	WAVEFORMATEX format{};
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = static_cast<DWORD>(waveform.sample_rate);
	format.wBitsPerSample = 16;
	format.nBlockAlign = sizeof(std::int16_t);
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	const HANDLE completion_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (completion_event == nullptr) return false;
	HWAVEOUT output = nullptr;
	const MMRESULT opened = waveOutOpen(&output, WAVE_MAPPER, &format,
		reinterpret_cast<DWORD_PTR>(completion_event), 0, CALLBACK_EVENT);
	if (opened != MMSYSERR_NOERROR) {
		CloseHandle(completion_event);
		return false;
	}
	WAVEHDR buffer{};
	buffer.lpData = reinterpret_cast<LPSTR>(waveform.samples.data());
	buffer.dwBufferLength = static_cast<DWORD>(waveform.samples.size() * sizeof(std::int16_t));
	const bool prepared = waveOutPrepareHeader(output, &buffer, sizeof(buffer)) == MMSYSERR_NOERROR;
	bool succeeded = prepared && waveOutWrite(output, &buffer, sizeof(buffer)) == MMSYSERR_NOERROR;
	if (succeeded) {
		while ((buffer.dwFlags & WHDR_DONE) == 0 && !cancellation.requested()) {
			if (WaitForSingleObject(completion_event, 20) == WAIT_FAILED) {
				succeeded = false;
				break;
			}
		}
		waveOutReset(output);
	}
	if (prepared) waveOutUnprepareHeader(output, &buffer, sizeof(buffer));
	waveOutClose(output);
	CloseHandle(completion_event);
	return succeeded;
}

} // namespace
#endif

class SpeechPlaybackService::PlaybackSession {
public:
#ifdef PROGEN3D_KOKORO_ENABLED
	~PlaybackSession()
	{
		{
			const std::lock_guard<std::mutex> lock(mutex);
			stopping = true;
			++program_revision;
		}
		wake_worker.notify_one();
		if (worker.joinable()) worker.join();
	}

	void run()
	{
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
		std::unique_ptr<KokoroSpeechEngine> engine;
		std::map<SpeechCacheKey, SpeechWaveform> cache;
		std::size_t cached_samples = 0;
		std::uint64_t cached_program = 0;
		for (;;) {
			std::unique_lock<std::mutex> lock(mutex);
			wake_worker.wait(lock, [&] { return stopping || !pending.empty() || !preparation.empty(); });
			if (stopping) return;
			const bool audible = !pending.empty();
			auto &queue = audible ? pending : preparation;
			const SpeechCacheKey speech = std::move(queue.front());
			queue.pop_front();
			SpeechCancellation cancellation{program_revision, playback_revision,
			                                program_revision.load(), playback_revision.load(), audible};
			lock.unlock();
			if (cached_program != cancellation.program) {
				cache.clear();
				cached_samples = 0;
				cached_program = cancellation.program;
			}
			try {
				if (engine == nullptr) engine = std::make_unique<KokoroSpeechEngine>(directory);
				auto found = cache.find(speech);
				SpeechWaveform generated;
				if (found == cache.end()) {
					if (!audible && cached_samples >= max_cached_samples) continue;
					generated = engine->synthesize({speech.second, speech.first},
					                               [&] { return cancellation.requested(); });
					if (cancellation.requested()) continue;
					if (generated.samples.size() <= max_cached_samples - cached_samples) {
						cached_samples += generated.samples.size();
						found = cache.emplace(speech, std::move(generated)).first;
					}
				}
				SpeechWaveform &waveform = found != cache.end() ? found->second : generated;
				if (audible && !cancellation.requested() && !playWaveform(waveform, cancellation))
					throw std::runtime_error("Windows could not play the speech cue.");
				lock.lock();
				if (!cancellation.requested())
					message = preparation.empty() ? "Kokoro ready" : "Preparing Kokoro speech";
			} catch (const std::exception &error) {
				if (!lock.owns_lock()) lock.lock();
				if (!cancellation.requested()) message = error.what();
				if (engine == nullptr) {
					pending.clear();
					preparation.clear();
					available = false;
				}
			}
		}
	}

	std::filesystem::path directory;
	mutable std::mutex mutex;
	std::condition_variable wake_worker;
	std::thread worker;
	std::deque<SpeechCacheKey> pending;
	std::deque<SpeechCacheKey> preparation;
	std::atomic<std::uint64_t> program_revision{0};
	std::atomic<std::uint64_t> playback_revision{0};
	bool stopping = false;
	bool available = false;
#endif
	std::string message = "Kokoro audio is unavailable in this build";
};

SpeechPlaybackService::SpeechPlaybackService() : session_(std::make_unique<PlaybackSession>()) {}

SpeechPlaybackService::~SpeechPlaybackService() = default;

void SpeechPlaybackService::configure(const std::filesystem::path &runtime_directory)
{
#ifdef PROGEN3D_KOKORO_ENABLED
	session_->directory = runtime_directory;
	for (const char *asset : {"sherpa-onnx-c-api.dll", "onnxruntime.dll", "model.int8.onnx",
	                         "voices.bin", "tokens.txt", "lexicon-us-en.txt"}) {
		std::error_code error;
		if (!std::filesystem::is_regular_file(runtime_directory / asset, error)) {
			session_->message = "Kokoro audio assets are missing";
			return;
		}
	}
	session_->available = true;
	session_->message = "Kokoro ready";
#else
	(void)runtime_directory;
#endif
}

void SpeechPlaybackService::prepare(const std::vector<AudioEventDefinition> &definitions)
{
#ifdef PROGEN3D_KOKORO_ENABLED
	const std::lock_guard<std::mutex> lock(session_->mutex);
	if (!session_->available) return;
	++session_->program_revision;
	session_->pending.clear();
	session_->preparation.clear();
	std::set<SpeechCacheKey> unique_cues;
	for (const auto &definition : definitions) {
		const auto &cue = definition.speech;
		SpeechCacheKey key(cue.speaker_id, cue.text);
		if (unique_cues.insert(key).second) session_->preparation.push_back(std::move(key));
		if (session_->preparation.size() >= max_prepared_cues) break;
	}
	session_->message = session_->preparation.empty() ? "Kokoro ready" : "Preparing Kokoro speech";
	if (session_->preparation.empty()) return;
	if (!session_->worker.joinable()) {
		try {
			session_->worker = std::thread(&PlaybackSession::run, session_.get());
		} catch (const std::system_error &error) {
			session_->message = error.what();
			session_->available = false;
			return;
		}
	}
	session_->wake_worker.notify_one();
#else
	(void)definitions;
#endif
}

void SpeechPlaybackService::play(const SpeechCue &speech)
{
#ifdef PROGEN3D_KOKORO_ENABLED
	const std::lock_guard<std::mutex> lock(session_->mutex);
	if (!session_->available || !session_->worker.joinable()) return;
	if (session_->pending.size() >= max_pending_speech) {
		session_->message = "Speech queue is full";
		return;
	}
	session_->pending.emplace_back(speech.speaker_id, speech.text);
	session_->wake_worker.notify_one();
#else
	(void)speech;
#endif
}

void SpeechPlaybackService::cancelPlayback()
{
#ifdef PROGEN3D_KOKORO_ENABLED
	const std::lock_guard<std::mutex> lock(session_->mutex);
	++session_->playback_revision;
	session_->pending.clear();
#endif
}

std::string SpeechPlaybackService::status() const
{
#ifdef PROGEN3D_KOKORO_ENABLED
	const std::lock_guard<std::mutex> lock(session_->mutex);
#endif
	return session_->message;
}
