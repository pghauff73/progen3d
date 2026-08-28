#pragma once

#include "lighting/model/LightId.h"

#include <string>
#include <utility>

enum class LightingValidationCode
{
	InvalidType,
	NonfiniteTransform,
	InvalidIntensity,
	InvalidRange,
	InvalidSpotCone,
	InvalidAreaSize,
	InvalidShadowResolution,
	DuplicateLightId,
	LightLimitExceeded,
	ShadowAllocationFailed
};

class LightingValidationIssue
{
public:
	LightingValidationIssue(LightingValidationCode code,
	                        LightId light_id,
	                        std::string message)
		: code_(code), light_id_(std::move(light_id)), message_(std::move(message)) {}

	LightingValidationCode code() const { return code_; }
	const LightId &lightId() const { return light_id_; }
	const std::string &message() const { return message_; }

private:
	LightingValidationCode code_ = LightingValidationCode::InvalidType;
	LightId light_id_;
	std::string message_;
};
