#include "grammar/reuse/GrammarReuseCatalogue.h"
#include "Context.h"
#include "GrammarRuntime.h"
#include "grammar.h"
#include "imgui_render.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

extern std::mt19937_64 grammar_rng;

static std::vector<float> generate(const std::string& source) {
    GrammarDocument document;
    setGrammarEvaluationDiagnosticsQuiet(&document,true);
    grammar_rng.seed(7);
    document.loadFromSourceText(source,"reuse test");
    if(!document.error_details.empty()) throw std::runtime_error(document.error_details.front().message);
    document.addContext();
    document.context->genPrimitives();
    document.generateGeometry();
    if(!document.error_details.empty()) throw std::runtime_error(document.error_details.front().message);
    const auto cube=build_base_vertex_data();
    std::vector<float> vertices, part;
    for(const auto& instance:document.context->primitive_instances){
        part.clear();
        document.context->buildInstanceBuffer(instance,cube.data(),&part);
        vertices.insert(vertices.end(),part.begin(),part.end());
    }
    if(vertices.empty()||!std::all_of(vertices.begin(),vertices.end(),[](float n){return std::isfinite(n);}))
        throw std::runtime_error("Empty model or nonfinite vertex.");
    return vertices;
}

static void require(bool result,const std::string& message){
    if(!result)throw std::runtime_error(message);
}

int main() {
    try{
        auto entries=GrammarReuseCatalogue::listEntries();
        require(entries.size()==419,"Wrong grammar count");
        for(const auto& entry:entries){
            const auto imported=generate("@use Example = \""+entry.reference+"\"\nStart -> Example");
            require(imported==generate(GrammarReuseCatalogue::readStandalone(entry.reference)),entry.reference);
        }
        const std::string ref="mechanics/bearing-family@1.0.0#model";
        const std::string header="@use Bearing = \""+ref+"\"";
        const std::string defaultSources=header+"\nStart -> Bearing";
        require(generate(defaultSources)!=generate(header+" with front=1 rear=1\nStart -> Bearing"),"Overrides ignored");
        const auto pair=generate(header+"\n@use Other = \""+ref+"\" with front=1 rear=1\nStart -> [ Bearing ] [ T(0.05 0 0) Other ]");
        require(pair.size()==generate(defaultSources).size()+generate(header+" with front=1 rear=1\nStart -> Bearing").size(),"Aliased instances");
        GrammarDocument broken;
        for(const std::string& input:std::vector<std::string>{
            header+" with count=7.5",header+" with family=5",header+" with count=7 count=7",
            "@use Bearing = \"bogus/name@1.0.0#missing\""}) {
            broken.loadFromSourceText("# source\n"+input+"\nStart -> Bearing","failure");
            require(!broken.error_details.empty() && broken.error_details.front().line==1 && broken.tokens_new.empty(),"Missing failure diagnostic");
        }
        broken.loadFromSourceText(defaultSources,"reread");
        const auto before=broken.tokens_new.size();broken.Reread();
        require(broken.error_details.empty() && before>0 && before==broken.tokens_new.size() &&
                !broken.lines.empty() && broken.lines.front()==header,"Reread lost source");
        std::cout<<"419 standalone/import native meshes match; overrides, alias isolation, error lines and reread passed.\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
