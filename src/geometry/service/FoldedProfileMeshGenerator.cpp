#include "geometry/service/FoldedProfileMeshGenerator.h"

#include "geometry/model/FoldedProfileShapeSpecification.h"
#include "geometry/model/SweepProfileShapeSpecification.h"
#include "geometry/service/Profile2DFactory.h"
#include "geometry/service/SweepProfileMeshGenerator.h"

GeometryBuildResult FoldedProfileMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *folded =
		dynamic_cast<const FoldedProfileShapeSpecification *>(&specification);
	if (folded == nullptr) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"FoldedProfileMeshGenerator requires a FoldedProfileShapeSpecification.");
	}
	std::string diagnostic;
	auto profile = Profile2DFactory(complexity_limits_).createRectangle(
		folded->thickness(), folded->extrusionDepth(), &diagnostic);
	if (!profile) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	std::vector<glm::vec3> path;
	path.reserve(folded->foldPath().size());
	for (const glm::vec2 &point : folded->foldPath()) {
		path.emplace_back(point.x, point.y, 0.0f);
	}
	SweepProfileShapeSpecification sweep(
		*profile,
		std::move(path),
		glm::vec3(0.0f, 0.0f, 1.0f),
		folded->capPolicy(),
		folded->key(),
		folded->canonicalText(),
		folded->detailLevel());
	return SweepProfileMeshGenerator(complexity_limits_).build(sweep);
}

GeneratedPrimitiveMesh FoldedProfileMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	GeometryBuildResult result = build(specification);
	if (!result.succeeded() && diagnostic != nullptr) {
		*diagnostic = result.firstDiagnostic();
	}
	return result.generatedMesh();
}
