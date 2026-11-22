#include "Context.h"
#include "GrammarRuntime.h"
#include "grammar.h"
#include "imgui_render.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <json/json.h>

namespace {
class GrammarTimeSampleAssertions
{
public:
    static void require(bool condition, const std::string &message)
    {
        if (!condition) throw std::runtime_error(message);
    }
    static void requireInvalidated(const GrammarDocument &grammar, const std::string &label)
    {
        GrammarRuntimeVariableSnapshot snapshot;
        require(grammar.tokens_new.empty(), label + ": prior actions survived rejection");
        require(!getGrammarRuntimeVariableSnapshot(&grammar, "t", &snapshot), label + ": prior time snapshot survived rejection");
    }
};

class NativeTimeSampleFixture
{
public:
    static void load(GrammarDocument &grammar, const std::string &source, double seconds = 0)
    {
        setGrammarEvaluationDiagnosticsQuiet(&grammar, true);
        GrammarTimeSampleAssertions::require(setGrammarEvaluationTime(&grammar, seconds), "Fixture time invalid");
        grammar.loadFromSourceText(source, "time-resampling-fixture");
    }
    static std::vector<float> geometry(GrammarDocument &grammar)
    {
        GrammarTimeSampleAssertions::require(grammar.error_details.empty(), "Fixture has grammar errors");
        grammar.addContext();
        grammar.context->genPrimitives();
        grammar.generateGeometry();
        GrammarTimeSampleAssertions::require(grammar.error_details.empty(), "Native fixture generation failed");
        const auto cube = build_base_vertex_data();
        std::vector<float> result;
        for (const auto &part : grammar.context->primitive_instances) {
            std::vector<float> buffer;
            grammar.context->buildInstanceBuffer(part, cube.data(), &buffer);
            GrammarTimeSampleAssertions::require(!buffer.empty(), "Native fixture buffer empty");
            for (float value : buffer) GrammarTimeSampleAssertions::require(std::isfinite(value), "Native fixture buffer nonfinite");
            result.insert(result.end(), buffer.begin(), buffer.end());
        }
        return result;
    }
    static void requireSourceRejected(const std::string &source, const std::string &label)
    {
        GrammarDocument grammar;
        load(grammar, source);
        if (grammar.error_details.empty()) {
            grammar.addContext(); grammar.context->genPrimitives(); grammar.generateGeometry();
        }
        GrammarTimeSampleAssertions::require(!grammar.error_details.empty(), label + ": invalid source was accepted");
    }
};

const std::string moving_source =
    "Start -> Move(t)\n"
    "Move(value) -> [ T(value 0 0) I(Sphere(segments(8 4)) material(softpinkmatteplaster) 1 1) ]\n";

void checkFreshAndPersistentGeometry()
{
    GrammarDocument persistent;
    NativeTimeSampleFixture::load(persistent, moving_source);
    std::vector<float> repeated;
    for (double seconds : {0.0, 2.0, 0.5, 2.0, 0.0}) {
        GrammarTimeSampleAssertions::require(persistent.reexpandAtTime(seconds), "Valid time expansion failed");
        const auto current = NativeTimeSampleFixture::geometry(persistent);
        GrammarDocument fresh;
        NativeTimeSampleFixture::load(fresh, moving_source, seconds);
        GrammarTimeSampleAssertions::require(current == NativeTimeSampleFixture::geometry(fresh), "Persistent and fresh actual geometry differ");
        GrammarTimeSampleAssertions::require(std::fabs(persistent.context->primitive_instances.front().primary_transform[3].x - seconds) < 1e-7,
                                            "Head-independent transform oracle failed");
        if (seconds == 2.0) {
            if (!repeated.empty()) GrammarTimeSampleAssertions::require(current == repeated, "Repeated time changed native geometry");
            repeated = current;
        }
    }
}

void checkInvalidTimeAndRecovery()
{
    GrammarDocument grammar;
    NativeTimeSampleFixture::load(grammar, moving_source);
    for (double seconds : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::max()}) {
        GrammarTimeSampleAssertions::require(!grammar.reexpandAtTime(seconds), "Invalid time was accepted");
        GrammarTimeSampleAssertions::requireInvalidated(grammar, "Invalid time");
        GrammarTimeSampleAssertions::require(grammar.reexpandAtTime(0.75), "Valid time did not recover after rejection");
        NativeTimeSampleFixture::geometry(grammar);
    }
}

void checkPublicRuleMutation()
{
    GrammarDocument grammar;
    NativeTimeSampleFixture::load(grammar, moving_source);
    auto rules = grammar.rule_list;
    grammar.rule_list.clear();
    GrammarTimeSampleAssertions::require(!grammar.reexpandAtTime(1), "Empty rule list was accepted");
    GrammarTimeSampleAssertions::requireInvalidated(grammar, "Empty rules");
    grammar.rule_list = rules;
    GrammarTimeSampleAssertions::require(grammar.reexpandAtTime(1), "Restored valid AST did not revalidate");
    const int move = grammar.findRule("Move");
    GrammarTimeSampleAssertions::require(move >= 0, "Move fixture missing");
    grammar.rule_list[move]->rule_name = "ChangedName";
    GrammarTimeSampleAssertions::require(!grammar.reexpandAtTime(2), "Semantically mutated rules were accepted");
    GrammarTimeSampleAssertions::requireInvalidated(grammar, "Semantic mutation");
    NativeTimeSampleFixture::load(grammar, moving_source);
    GrammarTimeSampleAssertions::require(grammar.reexpandAtTime(2), "Source reload did not recover semantic rejection");
    NativeTimeSampleFixture::geometry(grammar);
    NativeTimeSampleFixture::load(grammar, "Start -> MissingRule\n");
    GrammarTimeSampleAssertions::require(!grammar.reexpandAtTime(0), "Invalid parsed source was resampled");
    GrammarTimeSampleAssertions::requireInvalidated(grammar, "Invalid parsed source");
}

void checkGeometryFailureAndRecovery()
{
    const std::string source =
        "Start -> I(CubeY material(softpinkmatteplaster) 1 1) Face(0.5*t) I(Sphere(segments(8 4)) material(warmwhitematteplaster) 1 1)\n"
        "Face(activation) -> I(FacialSkin(response(elastic) bending(1) mouthMesh(quality) solve(8 .0001) "
        "muscle(left_smile .87 .56 34/48 20/64 .055 .08 activation .04 40)) material(softpinkmatteplaster) 1 1)\n";
    GrammarDocument grammar;
    NativeTimeSampleFixture::load(grammar, source, 1);
    GrammarTimeSampleAssertions::require(grammar.error_details.empty(), "Failure fixture should parse successfully");
    grammar.addContext(); grammar.context->genPrimitives(); grammar.generateGeometry();
    GrammarTimeSampleAssertions::require(!grammar.error_details.empty(), "Failed native face remained log-only");
    GrammarTimeSampleAssertions::require(grammar.error_details.front().message.find("did not converge") != std::string::npos,
                                        "Native solve diagnostic was lost");
    GrammarTimeSampleAssertions::require(grammar.error_details.front().line == 1, "Failure lost its source location");
    GrammarTimeSampleAssertions::require(grammar.context->primitive_instances.size() == 1, "Generation continued after failed face");
    GrammarTimeSampleAssertions::require(grammar.reexpandAtTime(0), "Valid time did not recover failed geometry");
    NativeTimeSampleFixture::geometry(grammar);
    GrammarTimeSampleAssertions::require(grammar.context->primitive_instances.size() == 3, "Recovered native scene incomplete");
    GrammarTimeSampleAssertions::require(grammar.context->lastPrimitiveDiagnostic().empty(), "Previous primitive error survived success");
}

void checkCurveNetworkGrammar()
{
    const std::string source = "Start -> I(CurveNetworkSurface("
        "uCurve(polyline 0 0 0 0 1 0) uCurve(polyline 1 0 0 1 1 0) "
        "vCurve(polyline 0 0 0 1 0 0) vCurve(polyline 0 1 0 1 1 0) "
        "samplesU(3) samplesV(3) tolerance(.0000001) method(interpolatingPatchGrid)) material(silvercyanpolishedmetal) 1 1)\n";
    GrammarDocument grammar;
    NativeTimeSampleFixture::load(grammar, source);
    NativeTimeSampleFixture::geometry(grammar);
    for (const auto &change : std::vector<std::pair<std::string, std::string>>{
            {"polyline 0", "polyline missingCoordinate"}, {"polyline", "polylin"},
            {"interpolatingPatchGrid", "interpolatingPatchGri"}}) {
        auto invalid = source;
        invalid.replace(invalid.find(change.first), change.first.size(), change.second);
        NativeTimeSampleFixture::requireSourceRejected(invalid, change.second);
    }
}
}

int main(int argc, char **argv)
{
    Json::Value report;
    report["schema"] = "progen3d.grammar-time-resampling-check.v1";
    report["checks"] = Json::Value(Json::arrayValue);
    try {
        const std::vector<std::pair<std::string, void (*)()>> checks = {
            {"freshAndPersistentActualGeometry", checkFreshAndPersistentGeometry},
            {"invalidTimeAndRecovery", checkInvalidTimeAndRecovery},
            {"publicRuleMutationRejection", checkPublicRuleMutation},
            {"nativeGeometryFailureAndRecovery", checkGeometryFailureAndRecovery},
            {"curveNetworkSymbolicAndNumericArguments", checkCurveNetworkGrammar}};
        for (const auto &[name, check] : checks) {
            check(); report["checks"].append(name);
        }
        report["passed"] = true;
    } catch (const std::exception &failure) {
        report["passed"] = false; report["error"] = failure.what();
    }
    Json::StreamWriterBuilder writer; writer["indentation"] = "  ";
    const auto text = Json::writeString(writer, report);
    std::cout << text << '\n';
    if (argc == 2) { std::ofstream output(argv[1]); output << text << '\n'; }
    return report["passed"].asBool() ? 0 : 1;
}
