#pragma once

enum class GrowthCurve
{
	Linear,
	SmoothStep,
	EaseIn,
	EaseOut
};

inline const char *growthCurveName(GrowthCurve curve)
{
	switch (curve) {
	case GrowthCurve::Linear: return "Linear";
	case GrowthCurve::SmoothStep: return "SmoothStep";
	case GrowthCurve::EaseIn: return "EaseIn";
	case GrowthCurve::EaseOut: return "EaseOut";
	}
	return "Unknown";
}
