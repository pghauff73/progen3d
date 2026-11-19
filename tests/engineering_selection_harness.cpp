#include "AppPaths.h"
#include "Context.h"
#include "editor/controller/SceneSelectionController.h"
#include "editor/service/GrammarCompilationService.h"
#include "geometry/service/AffineCubeFaceQuery.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char *message)
{
	if (!condition) throw std::runtime_error(message);
}

std::unique_ptr<GeneratedSceneSnapshot> compile(const std::string &part_body,
                                                 const std::string &sibling = "",
                                                 const std::string &identifier = "Bracket")
{
	SceneGenerationRequest request;
	request.source_text =
		"Start ->\n"
		"    Object(id(Assembly) class(Assembly) taxonomy(Engineering Sample)) [\n" +
		sibling +
		"        Object(id(" + identifier + ") class(Part) taxonomy(Engineering Part)) [\n" +
		part_body +
		"        ]\n"
		"    ]\n";
	auto result = GrammarCompilationService({}, {}).compile(request);
	if (!result->succeeded()) {
		for (const auto &diagnostic : result->diagnostics) std::cerr << diagnostic.message << '\n';
		throw std::runtime_error("Could not compile selection fixture");
	}
	return std::move(result->scene_snapshot);
}

} // namespace

int main(int argument_count, char **arguments)
{
	if (argument_count > 0) progen3d_initialize_paths(arguments[0]);
	try {
		const std::string part_body =
			"            T(3 1 5) A(90 2) S(-2 3 4) I(Cube\n"
			"                material(plaster))\n";
		auto before = compile(part_body);
		auto *context = before->generationContext();
		const auto &primitive = context->primitive_instances.at(0);
		const auto corners = cubeFaceWorldCorners(primitive, CubeFace::PositiveX);
		const auto center = (corners[0] + corners[2]) * 0.5f;
		require(cubeFaceAtWorldPoint(primitive, center) == CubeFace::PositiveX,
		        "Rotated and reflected cube lost its authored local face name");
		require(!cubeFaceAtWorldPoint(primitive, corners[0]), "A corner chose an arbitrary face");
		require(!cubeFaceAtWorldPoint(primitive, primitive.position), "An interior point had a face");

		SceneSelectionController controller;
		EditorSelection selection;
		controller.selectSpatialObject(selection, context, "Bracket");
		require(selection.selected_instance_index == 0 && controller.hasNamedCubeFaces(selection, context),
		        "Choosing a named object did not select its unique primitive");
		selection.hovered_instance_index = 0;
		controller.armPointerSelection(selection, 4.0f, 7.0f);
		require(controller.completePointerSelection(selection, 4.0f, 7.0f, 5.0f),
		        "A stationary click did not select the preview primitive");
		controller.selectPrimitive(selection, context, selection.selected_instance_index,
		                           cubeFaceAtWorldPoint(primitive, center));
		require(selection.selected_spatial_object_id == "Bracket" &&
		        selection.selected_face == CubeFace::PositiveX, "Preview selection lost its engineering anchor");

		const auto before_index = SceneSourceAssociationIndex::fromContext(*context);
		const auto source = controller.selectedSourceRange(
			selection, before_index, before.get(), before->sourceText());
		require(source.has_value() && source->start_line == 3 && source->end_line == 4 &&
		        source->start_column > 12 && source->end_column == 34,
		        "Source reveal did not find the exact multiline I instruction");
		require(!controller.selectedSourceRange(
			        selection, before_index, before.get(), "// draft\n" + before->sourceText()),
		        "The old preview was allowed to reveal a stale source span");

		const std::string sibling = "        [ T(50 0 0) I(Cube material(plaster)) ]\n";
		auto rebuilt = compile(part_body, sibling);
		context = rebuilt->generationContext();
		controller.rebindToScene(selection, context, false);
		require(selection.selected_instance_index == 1 &&
		        selection.selected_face == CubeFace::PositiveX, "Rebuild did not rebind the moved part by ID");
		const auto rebuilt_index = SceneSourceAssociationIndex::fromContext(*context);
		const auto new_source = controller.selectedSourceRange(
			selection, rebuilt_index, rebuilt.get(), rebuilt->sourceText());
		require(new_source.has_value() && new_source->start_line == 4 && new_source->end_line == 5,
		        "Rebuilt selection used an obsolete source span");
		controller.armPointerSelection(selection, 4.0f, 7.0f);
		controller.rebindToScene(selection, context, true);
		require(selection.pointer_selection_armed &&
		        selection.selected_instance_index == 1 &&
		        selection.selected_face == CubeFace::PositiveX,
		        "A timeline sample interrupted a click or lost its named anchor");

		auto split = compile(part_body + "            I(Cube material(plaster))\n", sibling);
		controller.rebindToScene(selection, split->generationContext(), false);
		require(selection.selected_spatial_object_id == "Bracket" &&
		        selection.selected_instance_index == -1 && !selection.selected_face,
		        "A split named part rebound to the wrong primitive or face");
		auto renamed = compile(part_body, sibling, "OtherBracket");
		controller.rebindToScene(selection, renamed->generationContext(), true);
		require(selection.selected_spatial_object_id.empty() &&
		        selection.selected_instance_index == -1 && !selection.selected_face,
		        "A missing named anchor fell back to a different primitive");
		controller.selectPrimitive(selection, split->generationContext(), 1, CubeFace::PositiveX);
		require(selection.selected_spatial_object_id == "Bracket" && !selection.selected_face,
		        "A compound part acquired an ambiguous face");
		controller.clear(selection);
		require(selection.selected_spatial_object_id.empty() && selection.selected_instance_index == -1,
		        "Document replacement retained the previous selection");
		std::cout << "Engineering selection passed picking, reflection, rebuild, source freshness, and split clearance.\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
