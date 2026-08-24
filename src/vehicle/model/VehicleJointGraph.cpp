#include "vehicle/model/VehicleJointGraph.h"

#include <algorithm>
#include <utility>

bool VehicleJointGraph::addJoint(VehicleJoint joint, std::string *diagnostic)
{
	const auto duplicate = std::find_if(
		joints_.begin(), joints_.end(),
		[&joint](const VehicleJoint &existing) {
			return existing.jointIdentifier() == joint.jointIdentifier();
		});
	if (duplicate != joints_.end()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Vehicle joint '" + joint.jointIdentifier() +
			              "' is declared more than once.";
		}
		return false;
	}
	joints_.push_back(std::move(joint));
	return true;
}
