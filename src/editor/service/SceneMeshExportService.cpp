#include "editor/service/MeshExportService.h"

bool MeshExportService::exportScene(const std::string &path,
	                                const SceneGenerationContext &scene_context,
	                                const GLfloat *base_vertex_data,
	                                MeshExportWriter &writer,
	                                std::string *error_message) const
{
	if (base_vertex_data == nullptr) {
		if (error_message != nullptr) {
			*error_message = "Mesh export requires primitive vertex data.";
		}
		return false;
	}
	return exportMesh(path,
	                  scene_context.buildExportMesh(base_vertex_data),
	                  writer,
	                  error_message);
}
