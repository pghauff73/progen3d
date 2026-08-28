#include "geometry/model/ShapeClosurePolicy.h"

#include <vector>

std::string ShapeClosurePolicy::canonicalText() const
{
	std::vector<std::string> names;
	if (axial_) names.push_back("axial");
	if (angular_) names.push_back("angular");
	if (clip_) names.push_back("clip");
	if (rim_) names.push_back("rim");
	if (names.empty()) return "none";
	if (names.size() == 4) return "all";

	std::string text;
	for (std::size_t index = 0; index < names.size(); ++index) {
		if (index > 0) text += ",";
		text += names[index];
	}
	return text;
}
