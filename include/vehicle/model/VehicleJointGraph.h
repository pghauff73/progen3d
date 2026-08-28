#pragma once

#include "vehicle/model/VehicleJoint.h"

#include <string>
#include <vector>

class VehicleJointGraph
{
public:
	bool addJoint(VehicleJoint joint, std::string *diagnostic);
	const std::vector<VehicleJoint> &joints() const { return joints_; }

private:
	std::vector<VehicleJoint> joints_;
};
