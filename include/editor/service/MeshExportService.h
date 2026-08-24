#pragma once

#include "Context.h"
#include "PLYWriter.h"

#include <string>

class MeshExportService
{
public:
	bool exportScene(const std::string &path,
	                 const SceneGenerationContext &scene_context,
	                 const GLfloat *base_vertex_data,
	                 MeshExportWriter &writer,
	                 std::string *error_message) const;
	bool exportMesh(const std::string &path,
	                const Mesh &mesh,
	                MeshExportWriter &writer,
	                std::string *error_message) const;
};
