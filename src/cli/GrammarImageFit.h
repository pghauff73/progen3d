#pragma once

#include "CalibratedCamera.h"
#include "editor/model/GeneratedSceneSnapshot.h"
#include "editor/model/PreviewImage.h"

#include <filesystem>
#include <functional>
#include <memory>

using CompileExperimentScene = std::function<std::unique_ptr<GeneratedSceneSnapshot>(
	const std::filesystem::path &, const std::string &, std::uint64_t, Json::Value &)>;
using RenderFitView = std::function<PreviewImage(
	const SceneGenerationContext &, const CalibratedCamera &)>;

bool fit_grammar_images(const std::filesystem::path &grammar_path, const std::filesystem::path &views_path,
                        const std::filesystem::path &destination, const CompileExperimentScene &compile,
                        const RenderFitView &render, Json::Value &summary);
