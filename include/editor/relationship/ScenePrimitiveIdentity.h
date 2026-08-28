#pragma once

class ScenePrimitiveIdentity
{
public:
	int instance_index = -1;

	bool isValid() const { return instance_index >= 0; }
};
