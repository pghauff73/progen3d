#include "editor/service/OpenGl46CapabilityValidator.h"

#include "ProGen3dGl.h"

bool OpenGl46CapabilityValidator::validate(std::string *error_message) const
{
	if (GLAD_GL_VERSION_4_6 != 0) {
		return true;
	}

	if (error_message != nullptr) {
		const GLubyte *version_text = glGetString(GL_VERSION);
		*error_message = "ProGen3D requires an OpenGL 4.6 Core context";
		if (version_text != nullptr) {
			*error_message += "; the active context reports ";
			*error_message += reinterpret_cast<const char *>(version_text);
		}
	}
	return false;
}
