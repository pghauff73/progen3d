#pragma once

enum class VegetationReferenceView
{
	Front,
	Right,
	Top
};

inline const char *vegetationReferenceViewName(VegetationReferenceView view)
{
	switch (view) {
	case VegetationReferenceView::Front: return "Front";
	case VegetationReferenceView::Right: return "Right";
	case VegetationReferenceView::Top: return "Top";
	}
	return "Unknown";
}
