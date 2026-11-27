#pragma once

#include "geometry/model/CalibratedOrthographicView.h"

#include <json/json.h>

class PreviewCameraState;

class CalibratedCamera {
public:
	static CalibratedCamera parse(const Json::Value &camera, int width, int height);
	const CalibratedOrthographicView &projection() const { return projection_; }
	PreviewCameraState preview() const;
	Json::Value metadata() const;

private:
	CalibratedOrthographicView projection_{};
};
