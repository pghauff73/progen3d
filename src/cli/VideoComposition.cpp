#include "VideoComposition.h"

#include "AppPaths.h"
#include "FfmpegProcess.h"
#include "VideoEdit.h"
#include "audio/service/KokoroSpeechEngine.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace {

namespace fs = std::filesystem;
constexpr int mix_rate = 48000;

struct CompositionFiles {
	fs::path video;
	fs::path narration;
	fs::path captions;
	fs::path manifest;
};

class CompositionWorkspace {
public:
	explicit CompositionWorkspace(const fs::path &output)
	{
		fs::create_directories(output);
		parent_ = fs::canonical(output);
		path_ = parent_ / (L".progen3d-compose-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
		                  std::to_wstring(GetTickCount64()));
		if (!fs::create_directory(path_)) throw std::runtime_error("Cannot create a composition workspace.");
	}

	~CompositionWorkspace()
	{
		std::error_code error;
		const auto resolved = fs::weakly_canonical(path_, error);
		if (!error && resolved == path_ && resolved.parent_path() == parent_) {
			fs::remove_all(resolved, error);
		}
	}

	CompositionWorkspace(const CompositionWorkspace &) = delete;
	CompositionWorkspace &operator=(const CompositionWorkspace &) = delete;

	const fs::path &path() const { return path_; }

	void publish(const CompositionFiles &files) const
	{
		const std::pair<fs::path, fs::path> artifacts[] = {
			{path_ / "narration.wav", files.narration}, {path_ / "captions.srt", files.captions},
			{path_ / "composition.json", files.manifest}, {path_ / "video.mp4", files.video}
		};
		std::size_t completed = 0;
		try {
			for (const auto &[source, destination] : artifacts) {
				if (!MoveFileW(source.c_str(), destination.c_str())) {
					throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
					                        "Publishing " + destination.u8string());
				}
				++completed;
			}
		} catch (...) {
			for (std::size_t index = 0; index < completed; ++index) {
				std::error_code ignored;
				fs::remove(artifacts[index].second, ignored);
			}
			throw;
		}
	}

private:
	fs::path parent_;
	fs::path path_;
};

CompositionFiles output_files(const fs::path &output)
{
	const auto video = fs::absolute(output).lexically_normal();
	auto extension = video.extension().wstring();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t ch) {
		return static_cast<wchar_t>(std::towlower(ch));
	});
	if (extension != L".mp4") throw std::runtime_error("The compose output path must end in .mp4.");
	CompositionFiles files{video, video, video, video};
	files.narration.replace_extension(".wav");
	files.captions.replace_extension(".srt");
	files.manifest.replace_extension(".composition.json");
	for (const auto &path : {files.video, files.narration, files.captions, files.manifest}) {
		if (fs::exists(path) || fs::is_symlink(path)) {
			throw std::runtime_error("The composition output already exists: " + path.u8string());
		}
	}
	return files;
}

void write_text(const fs::path &path, const std::string &text)
{
	std::ofstream output(path, std::ios::binary);
	output.exceptions(std::ios::badbit | std::ios::failbit);
	output.write(text.data(), static_cast<std::streamsize>(text.size()));
	output.close();
}

void stage_file(const fs::path &source, const fs::path &destination)
{
	if (!fs::is_regular_file(source)) throw std::runtime_error("Missing media file: " + source.u8string());
	std::error_code error;
	fs::create_hard_link(source, destination, error);
	if (error) fs::copy_file(source, destination);
}

void stage_frames(const VideoEdit &edit, const fs::path &folder)
{
	for (std::size_t index = 0; index < edit.frames.size(); ++index) {
		const auto &source = edit.frames[index];
		if (!fs::is_regular_file(source)) throw std::runtime_error("Missing saved frame: " + source.u8string());
		std::ifstream input(source, std::ios::binary);
		std::string format, dimensions, levels;
		std::getline(input, format);
		std::getline(input, dimensions);
		std::getline(input, levels);
		std::istringstream size(dimensions);
		int width = 0, height = 0;
		std::string extra;
		const auto offset = input.tellg();
		if ((format != "P6" && format != "P6\r") || (levels != "255" && levels != "255\r") ||
		    !(size >> width >> height) || (size >> extra) || width != edit.width ||
		    height != edit.height || offset < 0 ||
		    fs::file_size(source) != static_cast<std::uintmax_t>(static_cast<std::streamoff>(offset)) +
		                            static_cast<std::uintmax_t>(width) * height * 3) {
			throw std::runtime_error("Expected a saved P6 frame matching the edit size: " + source.u8string());
		}
		std::ostringstream name;
		name << "frame-" << std::setw(8) << std::setfill('0') << index << ".ppm";
		stage_file(source, folder / name.str());
	}
}

std::string subtitle_time(std::uint32_t frame, int fps)
{
	const auto ms = (static_cast<std::uint64_t>(frame) * 1000 + fps / 2) / fps;
	std::ostringstream result;
	result.imbue(std::locale::classic());
	result << std::setfill('0') << std::setw(2) << ms / 3600000 << ':' << std::setw(2) << ms / 60000 % 60
	       << ':' << std::setw(2) << ms / 1000 % 60 << ',' << std::setw(3) << ms % 1000;
	return result.str();
}

std::string overlay_text(std::string text)
{
	text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
	return text;
}

void write_captions(const VideoEdit &edit, const fs::path &folder)
{
	std::ostringstream captions;
	captions.imbue(std::locale::classic());
	std::size_t index = 0;
	for (const auto &caption : edit.captions) {
		auto text = overlay_text(caption.text);
		const auto first = text.find_first_not_of("\n");
		if (first == std::string::npos) continue;
		text = text.substr(first, text.find_last_not_of("\n") - first + 1);
		text.erase(std::unique(text.begin(), text.end(), [](char a, char b) {
			return a == '\n' && b == '\n';
		}), text.end());
		captions << ++index << '\n' << subtitle_time(caption.start_frame, edit.fps) << " --> "
		         << subtitle_time(caption.end_frame, edit.fps) << '\n' << text << "\n\n";
	}
	write_text(folder / "captions.srt", captions.str());
}

void write_video_filter(const VideoEdit &edit, const fs::path &folder)
{
	std::ostringstream graph;
	graph.imbue(std::locale::classic());
	graph << "pad=ceil(iw/2)*2:ceil(ih/2)*2";
	if (!edit.titles.empty() || !edit.captions.empty()) {
		wchar_t windows[MAX_PATH];
		const auto length = GetWindowsDirectoryW(windows, MAX_PATH);
		if (!length || length >= MAX_PATH) throw std::runtime_error("Cannot locate the Windows fonts.");
		stage_file(fs::path(windows) / "Fonts" / "segoeui.ttf", folder / "overlay.ttf");
	}
	std::size_t index = 0;
	const auto add_overlay = [&](const EditText &item, bool title) {
		const std::string filename = "overlay-" + std::to_string(index++) + ".txt";
		write_text(folder / filename, overlay_text(item.text));
		const int size = std::max(12, edit.height / (title ? 13 : 18));
		const int margin = std::max(8, edit.height / 18);
		graph << ",drawtext=fontfile=overlay.ttf:textfile=" << filename
		      << ":expansion=none:fontsize=" << size << ":fontcolor=white"
		      << ":box=1:boxcolor=black@0.74:boxborderw=" << std::max(3, size / 4)
		      << ":line_spacing=4:fix_bounds=1:x=";
		if (title) graph << margin << ":y=" << margin;
		else graph << "(w-tw)/2:y=h-th-" << margin;
		graph << ":enable='between(n," << item.start_frame << ',' << item.end_frame - 1 << ")'";
	};
	for (const auto &title : edit.titles) add_overlay(title, true);
	for (const auto &caption : edit.captions) add_overlay(caption, false);
	write_text(folder / "video.filter", graph.str());
}

void write_speech_wav(const fs::path &path, const SpeechWaveform &wave)
{
	const auto onset = std::find_if(wave.samples.begin(), wave.samples.end(), [](std::int16_t sample) {
		return std::abs(static_cast<int>(sample)) > 16;
	});
	if (onset == wave.samples.end()) throw std::runtime_error("Speech synthesis produced silence.");
	const auto lead = static_cast<std::size_t>(onset - wave.samples.begin());
	const auto preroll = static_cast<std::size_t>(wave.sample_rate / 200);
	const auto start = lead > preroll ? lead - preroll : 0;
	const auto count = wave.samples.size() - start;
	if (count > (std::numeric_limits<std::uint32_t>::max() - 36) / 2) {
		throw std::runtime_error("Synthesized narration is too large for WAV.");
	}

	std::ofstream output(path, std::ios::binary);
	output.exceptions(std::ios::badbit | std::ios::failbit);
	const auto put_integer = [&](std::uint32_t value, int bytes) {
		for (int index = 0; index < bytes; ++index) {
			output.put(static_cast<char>((value >> (8 * index)) & 255));
		}
	};
	const auto sample_bytes = static_cast<std::uint32_t>(count) * 2;
	output.write("RIFF", 4);
	put_integer(36 + sample_bytes, 4);
	output.write("WAVEfmt ", 8);
	put_integer(16, 4);
	put_integer(1, 2);
	put_integer(1, 2);
	put_integer(wave.sample_rate, 4);
	put_integer(static_cast<std::uint32_t>(wave.sample_rate) * 2, 4);
	put_integer(2, 2);
	put_integer(16, 2);
	output.write("data", 4);
	put_integer(sample_bytes, 4);
	// The Windows CLI writes little-endian PCM.
	output.write(reinterpret_cast<const char *>(wave.samples.data() + start),
	             static_cast<std::streamsize>(sample_bytes));
	output.close();
}

std::int64_t audio_sample(double seconds)
{
	return std::llround(seconds * mix_rate);
}

Json::Value mix_narration(const VideoEdit &edit, const fs::path &folder, const FfmpegProcess &ffmpeg)
{
	std::vector<std::wstring> arguments{L"-f", L"lavfi", L"-i", L"anullsrc=sample_rate=48000:channel_layout=stereo"};
	std::ostringstream graph;
	graph.imbue(std::locale::classic());
	graph << std::setprecision(17);
	const auto total_samples = audio_sample(edit.duration());
	graph << "[0:a]atrim=end_sample=" << total_samples << ",aformat=sample_fmts=fltp[bed];\n";

	std::unique_ptr<KokoroSpeechEngine> engine;
	std::map<std::pair<int, std::string>, std::string> synthesized;
	std::size_t inputs = 0;
	Json::Value placed(Json::arrayValue);
	for (const auto &clip : edit.narration) {
		const auto start = audio_sample(clip.start_seconds);
		const auto end = audio_sample(clip.end_seconds);
		if (end <= start) continue;
		++inputs;
		std::string filename;
		if (clip.wav) {
			filename = "narration-" + std::to_string(inputs) + ".wav";
			stage_file(*clip.wav, folder / filename);
		} else {
			const auto key = std::make_pair(clip.speech.speaker_id, clip.speech.text);
			const auto found = synthesized.find(key);
			if (found != synthesized.end()) {
				filename = found->second;
			} else {
				if (!engine) engine = std::make_unique<KokoroSpeechEngine>(progen3d_resource_path("audio/kokoro"));
				filename = "speech-" + std::to_string(inputs) + ".wav";
				write_speech_wav(folder / filename, engine->synthesize(clip.speech));
				synthesized.emplace(key, filename);
			}
		}
		arguments.insert(arguments.end(), {L"-f", L"wav", L"-i", fs::path(filename).wstring()});
		graph << '[' << inputs << ":a]aresample=" << mix_rate
		      << ",aformat=sample_fmts=fltp:channel_layouts=stereo,atrim=end_sample=" << end - start
		      << ",asetpts=PTS-STARTPTS,volume=" << clip.gain << ",adelay=delays=" << start
		      << "S:all=1[voice" << inputs << "];\n";

		Json::Value entry;
		entry["start_sample"] = static_cast<Json::Int64>(start);
		entry["end_limit_sample"] = static_cast<Json::Int64>(end);
		entry["start_frame"] = static_cast<Json::Int64>(std::llround(clip.start_seconds * edit.fps));
		entry["time_seconds"] = clip.start_seconds;
		entry["trigger"] = clip.trigger;
		if (clip.wav) {
			entry["wav"] = clip.wav->u8string();
		} else {
			entry["id"] = clip.id;
			entry["text"] = clip.speech.text;
			entry["speaker_id"] = clip.speech.speaker_id;
		}
		placed.append(std::move(entry));
	}

	graph << "[bed]";
	for (std::size_t input = 1; input <= inputs; ++input) graph << "[voice" << input << ']';
	if (inputs) graph << "amix=inputs=" << inputs + 1 << ":duration=first:normalize=0,";
	graph << "atrim=end_sample=" << total_samples << "[mixed]\n";
	write_text(folder / "audio.filter", graph.str());
	arguments.insert(arguments.end(), {L"-/filter_complex", L"audio.filter", L"-map", L"[mixed]",
	                                   L"-c:a", L"pcm_s16le", L"narration.wav"});
	ffmpeg.run(arguments, "mix the narration");
	return placed;
}

Json::Value composition_report(const VideoEdit &edit, const CompositionFiles &files, Json::Value narration)
{
	Json::Value result;
	result["schema_version"] = 1;
	result["video"] = files.video.u8string();
	result["narration_wav"] = files.narration.u8string();
	result["captions_srt"] = files.captions.u8string();
	result["composition_manifest"] = files.manifest.u8string();
	result["frame_count"] = static_cast<Json::UInt64>(edit.frames.size());
	result["fps"] = edit.fps;
	result["width"] = (edit.width + 1) / 2 * 2;
	result["height"] = (edit.height + 1) / 2 * 2;
	result["duration_seconds"] = edit.duration();
	result["audio_sample_rate"] = mix_rate;
	result["narration"] = std::move(narration);
	result["cuts"] = Json::Value(Json::arrayValue);
	for (const auto &cut : edit.cuts) {
		Json::Value entry;
		entry["sequence"] = cut.sequence.u8string();
		entry["start_frame"] = cut.start_frame;
		entry["end_frame"] = cut.end_frame;
		entry["output_start_frame"] = cut.output_start_frame;
		result["cuts"].append(std::move(entry));
	}
	return result;
}

}

Json::Value compose_video(const VideoEdit &edit, const fs::path &video_path)
{
	const auto files = output_files(video_path);
	CompositionWorkspace workspace(files.video.parent_path());
	const auto &folder = workspace.path();
	const FfmpegProcess ffmpeg(folder);

	stage_frames(edit, folder);
	write_video_filter(edit, folder);
	write_captions(edit, folder);
	auto narration = mix_narration(edit, folder, ffmpeg);
	ffmpeg.run({L"-f", L"image2", L"-framerate", std::to_wstring(edit.fps), L"-start_number", L"0",
	            L"-i", L"frame-%08d.ppm", L"-f", L"wav", L"-i", L"narration.wav", L"-map", L"0:v:0",
	            L"-map", L"1:a:0", L"-/filter:v", L"video.filter", L"-c:v", L"libx264", L"-preset",
	            L"fast", L"-crf", L"18", L"-pix_fmt", L"yuv420p", L"-c:a", L"aac", L"-b:a", L"160k",
	            L"-movflags", L"+faststart", L"video.mp4"}, "encode the video");

	auto report = composition_report(edit, files, std::move(narration));
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "  ";
	write_text(folder / "composition.json", Json::writeString(writer, report) + "\n");
	workspace.publish(files);
	return report;
}
