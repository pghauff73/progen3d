#pragma once

#include <json/json.h>

#include <filesystem>

void resolve_render_effect_inputs(Json::Value &job, const std::filesystem::path &directory);
