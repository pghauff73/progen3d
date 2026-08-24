#pragma once

#include "editor/model/LocalFileDialogRequest.h"
#include "editor/model/LocalFileDialogResult.h"

class LocalFileDialogService
{
public:
	virtual ~LocalFileDialogService() = default;
	virtual LocalFileDialogResult show(const LocalFileDialogRequest &request) = 0;
};
