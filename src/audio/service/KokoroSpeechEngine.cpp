#include "audio/service/KokoroSpeechEngine.h"

#include <stdexcept>

#ifdef PROGEN3D_KOKORO_ENABLED
#include <windows.h>
#include <sherpa-onnx/c-api/c-api.h>

#include <algorithm>
#include <cmath>
#include <cstring>

class KokoroSpeechEngine::Runtime {
public:
	explicit Runtime(const std::filesystem::path &directory)
	{
		const auto library_path = directory / "sherpa-onnx-c-api.dll";
		library_ = LoadLibraryExW(library_path.c_str(), nullptr,
		                         LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (library_ == nullptr) throw std::runtime_error("Unable to load the Kokoro runtime DLL.");
		try {
			initialize(directory);
		} catch (...) {
			FreeLibrary(library_);
			throw;
		}
	}

	~Runtime()
	{
		destroy_engine_(engine_);
		FreeLibrary(library_);
	}

	SpeechWaveform synthesize(const SpeechCue &speech, const std::function<bool()> &cancelled) const
	{
		if (speech.speaker_id < 0 || speech.speaker_id >= num_speakers_(engine_))
			throw std::runtime_error("Kokoro does not have speaker " + std::to_string(speech.speaker_id) + ".");
		SherpaOnnxGenerationConfig generation{};
		generation.sid = speech.speaker_id;
		generation.speed = 1.0f;
		generation.silence_scale = 0.2f;
		std::function<bool()> should_cancel = [&] { return cancelled && cancelled(); };
		const std::unique_ptr<const SherpaOnnxGeneratedAudio, decltype(destroy_audio_)> audio(
			generate_(engine_, speech.text.c_str(), &generation,
			          [](const float *, int32_t, float, void *context) -> int32_t {
			          return (*static_cast<std::function<bool()> *>(context))() ? 0 : 1;
		          }, &should_cancel), destroy_audio_);
		if (should_cancel()) return {};
		if (audio == nullptr || audio->samples == nullptr || audio->n <= 0 || audio->sample_rate <= 0)
			throw std::runtime_error("Kokoro could not synthesize an audio cue.");
		SpeechWaveform waveform;
		waveform.sample_rate = audio->sample_rate;
		waveform.samples.reserve(static_cast<std::size_t>(audio->n));
		for (int32_t index = 0; index < audio->n; ++index) {
			const float sample = audio->samples[index];
			waveform.samples.push_back(std::isfinite(sample)
				? static_cast<std::int16_t>(std::lround(std::clamp(sample, -1.0f, 1.0f) * 32767.0f))
				: 0);
		}
		return waveform;
	}

private:
	template <typename Function>
	Function loadFunction(const char *name) const
	{
		const FARPROC symbol = GetProcAddress(library_, name);
		if (symbol == nullptr) throw std::runtime_error("The Kokoro runtime DLL is incompatible.");
		Function function;
		static_assert(sizeof(function) == sizeof(symbol));
		std::memcpy(&function, &symbol, sizeof(function));
		return function;
	}

	void initialize(const std::filesystem::path &directory)
	{
		const auto version = loadFunction<decltype(&SherpaOnnxGetVersionStr)>("SherpaOnnxGetVersionStr");
		if (std::string(version()) != "2.0.3")
			throw std::runtime_error("Kokoro requires the staged Sherpa-ONNX 2.0.3 runtime.");
		const auto create = loadFunction<decltype(&SherpaOnnxCreateOfflineTts)>("SherpaOnnxCreateOfflineTts");
		destroy_engine_ = loadFunction<decltype(destroy_engine_)>("SherpaOnnxDestroyOfflineTts");
		num_speakers_ = loadFunction<decltype(num_speakers_)>("SherpaOnnxOfflineTtsNumSpeakers");
		generate_ = loadFunction<decltype(generate_)>("SherpaOnnxOfflineTtsGenerateWithConfig");
		destroy_audio_ = loadFunction<decltype(destroy_audio_)>("SherpaOnnxDestroyOfflineTtsGeneratedAudio");

		const std::string model = (directory / "model.int8.onnx").u8string();
		const std::string voices = (directory / "voices.bin").u8string();
		const std::string tokens = (directory / "tokens.txt").u8string();
		const std::string lexicon = (directory / "lexicon-us-en.txt").u8string();
		SherpaOnnxOfflineTtsConfig config{};
		config.model.kokoro.model = model.c_str();
		config.model.kokoro.voices = voices.c_str();
		config.model.kokoro.tokens = tokens.c_str();
		config.model.kokoro.lexicon = lexicon.c_str();
		config.model.provider = "cpu";
		config.model.num_threads = 2;
		config.max_num_sentences = 1;
		engine_ = create(&config);
		if (engine_ == nullptr) throw std::runtime_error("Unable to initialize the Kokoro model.");
	}

	HMODULE library_ = nullptr;
	const SherpaOnnxOfflineTts *engine_ = nullptr;
	decltype(&SherpaOnnxDestroyOfflineTts) destroy_engine_ = nullptr;
	decltype(&SherpaOnnxOfflineTtsNumSpeakers) num_speakers_ = nullptr;
	decltype(&SherpaOnnxOfflineTtsGenerateWithConfig) generate_ = nullptr;
	decltype(&SherpaOnnxDestroyOfflineTtsGeneratedAudio) destroy_audio_ = nullptr;
};

#else
class KokoroSpeechEngine::Runtime {};
#endif

KokoroSpeechEngine::KokoroSpeechEngine(const std::filesystem::path &directory)
{
#ifdef PROGEN3D_KOKORO_ENABLED
	runtime_ = std::make_unique<Runtime>(directory);
#else
	(void)directory;
	throw std::runtime_error("This build lacks Kokoro. Provide WAV narration or build with the Kokoro bundle.");
#endif
}

KokoroSpeechEngine::~KokoroSpeechEngine() = default;

SpeechWaveform KokoroSpeechEngine::synthesize(const SpeechCue &speech,
                                            const std::function<bool()> &cancelled) const
{
#ifdef PROGEN3D_KOKORO_ENABLED
	return runtime_->synthesize(speech, cancelled);
#else
	(void)speech;
	(void)cancelled;
	throw std::runtime_error("This build lacks Kokoro.");
#endif
}
