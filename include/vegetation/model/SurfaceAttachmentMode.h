#pragma once

enum class SurfaceAttachmentMode
{
	Contact,
	Offset,
	Twine
};

inline const char *surfaceAttachmentModeName(SurfaceAttachmentMode mode)
{
	switch (mode) {
	case SurfaceAttachmentMode::Contact: return "Contact";
	case SurfaceAttachmentMode::Offset: return "Offset";
	case SurfaceAttachmentMode::Twine: return "Twine";
	}
	return "Unknown";
}
