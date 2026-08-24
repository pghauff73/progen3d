#include "editor/service/MeshExportService.h"

bool MeshExportService::exportMesh(const std::string &path,
	                               const Mesh &mesh,
	                               MeshExportWriter &writer,
	                               std::string *error_message) const
{
	if (path.empty()) {
		if (error_message != nullptr) {
			*error_message = "Choose a path for the mesh export.";
		}
		return false;
	}
	if (mesh.vertices.empty() || mesh.faces.empty()) {
		if (error_message != nullptr) {
			*error_message = "The last valid scene does not contain exportable triangles.";
		}
		return false;
	}
	if (!writer.writeMesh(path, mesh)) {
		if (error_message != nullptr) {
			*error_message = "The mesh writer could not create the export file: " + path;
		}
		return false;
	}
	return true;
}
