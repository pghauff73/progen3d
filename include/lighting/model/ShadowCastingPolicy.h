#pragma once

#include <cstdint>

enum class ShadowUpdateFrequency
{
	EveryFrame,
	OnTransformChange,
	OnSceneChange,
	Static
};

class ShadowCastingPolicy
{
public:
	bool enabled() const { return enabled_; }
	void setEnabled(bool enabled) { enabled_ = enabled; }

	std::uint32_t resolution() const { return resolution_; }
	void setResolution(std::uint32_t resolution) { resolution_ = resolution; }

	float softness() const { return softness_; }
	void setSoftness(float softness) { softness_ = softness; }

	float constantBias() const { return constant_bias_; }
	void setConstantBias(float constant_bias) { constant_bias_ = constant_bias; }

	float slopeBias() const { return slope_bias_; }
	void setSlopeBias(float slope_bias) { slope_bias_ = slope_bias; }

	float normalBias() const { return normal_bias_; }
	void setNormalBias(float normal_bias) { normal_bias_ = normal_bias; }

	ShadowUpdateFrequency updateFrequency() const { return update_frequency_; }
	void setUpdateFrequency(ShadowUpdateFrequency update_frequency)
	{
		update_frequency_ = update_frequency;
	}

private:
	bool enabled_ = false;
	std::uint32_t resolution_ = 1024;
	float softness_ = 1.0f;
	float constant_bias_ = 0.0005f;
	float slope_bias_ = 1.5f;
	float normal_bias_ = 0.0f;
	ShadowUpdateFrequency update_frequency_ = ShadowUpdateFrequency::EveryFrame;
};
