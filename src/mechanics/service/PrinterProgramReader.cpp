#include "mechanics/service/PrinterResponse.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace fff {
bool PrinterProgramReader::isDirective(const std::string &line) {
    const auto at=line.find_first_not_of(" \t\r");
    return at!=std::string::npos && line.compare(at,4,"@fff")==0 &&
        (at+4==line.size() || std::isspace(static_cast<unsigned char>(line[at+4])));
}
PrinterProgram PrinterProgramReader::read(std::vector<std::string> &lines, int &where) {
    const std::map<std::string,size_t> arities={{"beltX",13},{"beltY",13},{"gear",9},{"guide",4},
        {"stockMaterial",5},{"beltCreep",4},{"thermal",3},{"nut",1},{"hotend",5},{"bead",2},{"home",3},{"move",5}};
    PrinterProgram p;
    std::set<std::string> seen;
    std::vector<double> creep;
    for (size_t i=0;i<lines.size();++i) {
        if (!isDirective(lines[i])) continue;
        where=static_cast<int>(i);
        std::istringstream input(lines[i].substr(0,lines[i].find('#')));
        std::string prefix, kind;
        input>>prefix>>kind;
        auto arity=arities.find(kind);
        if (arity==arities.end()) throw std::invalid_argument("unknown record '"+kind+"'");
        const bool first=seen.insert(kind).second;
        if (!first && kind!="move") throw std::invalid_argument("duplicate "+kind);
        std::vector<double> a(arity->second);
        for (double &v:a) if (!(input>>v) || !std::isfinite(v))
            throw std::invalid_argument("invalid or missing numeric field in "+kind);
        std::string extra;
        if (input>>extra) throw std::invalid_argument("extra field in "+kind);
        if (kind=="beltX" || kind=="beltY") {
            auto &belt=kind=="beltX" ? p.xDrive : p.yDrive;
            belt={a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10],a[11],a[12]};
        } else if (kind=="gear") p.gears={a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]};
        else if (kind=="guide") p.guides={a[0],a[1],a[2],a[3]};
        else if (kind=="thermal") p.thermal={a[0],a[1],a[2]};
        else if (kind=="hotend") p.hotend={a[0],a[1],a[2],a[3],a[4]};
        else if (kind=="stockMaterial") p.stock=StockMaterial{a[0],a[1],a[2],a[3],a[4]};
        else if (kind=="beltCreep") creep=a;
        else if (kind=="nut") p.nutBacklashMm=a[0];
        else if (kind=="bead") { p.widthMm=a[0]; p.heightMm=a[1]; }
        else if (kind=="home") p.home={a[0],a[1],a[2]};
        else p.moves.push_back({a[0],{a[1],a[2],a[3]},a[4]});
        lines[i].clear();
    }
    if (!creep.empty()) {
        p.xDrive.creepFraction=creep[0]; p.xDrive.retardationS=creep[1];
        p.yDrive.creepFraction=creep[2]; p.yDrive.retardationS=creep[3];
    }
    for (const auto &entry:arities)
        if (entry.first!="stockMaterial" && entry.first!="beltCreep" && !seen.count(entry.first))
        throw std::invalid_argument("missing "+entry.first+" record");
    return p;
}
}
