#pragma once

#include "editor/model/ApplicationLog.h"

struct ImFont;

class ConsolePanel
{
public:
	void draw(ApplicationLog &application_log, ImFont *monospace_font) const;
};
