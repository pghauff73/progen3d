#pragma once

#include "ExperimentParameters.h"
#include "editor/model/PreviewImage.h"

#include <functional>
#include <optional>

class RenderSweep {
public:
	explicit RenderSweep(const std::filesystem::path &manifest);

	using RenderVariant = std::function<PreviewImage(const std::string &, std::optional<float>, Json::Value &)>;
	bool run(const std::filesystem::path &grammar, const std::filesystem::path &destination,
	         const Json::Value &capture_settings, const RenderVariant &render, Json::Value &summary) const;

	const std::filesystem::path &job() const { return job_; }
	const std::optional<std::string> &object() const { return object_; }
	bool variesAperture() const { return apertures_.front().has_value(); }

private:
	std::filesystem::path manifest_;
	std::filesystem::path job_;
	ParameterGrid parameters_;
	std::vector<std::optional<float>> apertures_;
	std::optional<std::string> object_;
};
