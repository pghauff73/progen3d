#include "grammar/reuse/GrammarReuseCatalogue.h"
#include "AppPaths.h"
#include <sqlite3.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>

namespace {
using Database = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

Database openDatabase() {
    const char* overridePath = std::getenv("PROGEN3D_REUSE_DB");
    const std::string path = overridePath && *overridePath ? overridePath :
        progen3d_resource_path("resources/reuse/catalogue.sqlite3").string();
    sqlite3* raw = nullptr;
    const int status = sqlite3_open_v2(path.c_str(), &raw, SQLITE_OPEN_READONLY, nullptr);
    Database database(raw, sqlite3_close);
    if (status != SQLITE_OK) throw std::runtime_error("Cannot open grammar catalogue at " + path);
    return database;
}

Statement prepare(sqlite3* db, const char* sql, const std::string* value = nullptr) {
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db));
    Statement statement(raw, sqlite3_finalize);
    if (value && sqlite3_bind_text(raw, 1, value->c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db));
    return statement;
}

bool nextRow(sqlite3_stmt* row) {
    const int status = sqlite3_step(row);
    if (status == SQLITE_ROW) return true;
    if (status == SQLITE_DONE) return false;
    throw std::runtime_error(sqlite3_errmsg(sqlite3_db_handle(row)));
}

std::string column(sqlite3_stmt* statement, int index) {
    const unsigned char* bytes = sqlite3_column_text(statement, index);
    return bytes ? std::string(reinterpret_cast<const char*>(bytes),sqlite3_column_bytes(statement,index)) : "";
}

std::string readGrammar(sqlite3* db, const std::string& ref) {
    auto row = prepare(db,"SELECT grammar FROM standalone WHERE ref=?",&ref);
    if (!nextRow(row.get())) throw std::runtime_error("Unknown grammar reference " + ref);
    return column(row.get(),0);
}

struct CatalogueImport {
    int line;
    std::string alias;
    std::string reference;
    std::map<std::string,double> overrides;
};

CatalogueImport parseImport(const std::string& line, int number) {
    // Match only the declaration head; the reference may contain a literal #.
    static const std::regex declaration(R"re(^\s*@use\s+([A-Za-z][A-Za-z0-9_]*)\s*=\s*"([a-z][a-z0-9-]*(/[a-z][a-z0-9-]*)+@[0-9]+\.[0-9]+\.[0-9]+#[a-z0-9][a-z0-9-]*)")re");
    static const std::regex assignment(R"(^([A-Za-z_][A-Za-z_0-9]*)=([+-]?(?:[0-9]+\.?[0-9]*|\.[0-9]+)(?:[Ee][+-]?[0-9]+)?)$)");
    std::smatch match;
    if (line.size()>4096 || !std::regex_search(line,match,declaration))
        throw GrammarImportError(number,"Use @use Alias = \"package/name@version#export\".");
    CatalogueImport item{number,match[1],match[2],{}};
    std::string remainder=match.suffix().str();
    if (remainder.find_first_not_of(" \t\r") == std::string::npos) return item;
    const auto comment = std::min(remainder.find('#'),remainder.find("//"));
    remainder = remainder.substr(0,comment);
    std::istringstream tokens(remainder);
    std::string word;
    if (!(tokens>>word)) return item;
    if (word!="with" || !std::isspace(static_cast<unsigned char>(remainder[0])))
        throw GrammarImportError(number,"Use 'with name=value' after a grammar reference.");
    bool found=false;
    while(tokens>>word){
        if (!std::regex_match(word,match,assignment)) throw GrammarImportError(number,"Invalid parameter: "+word);
        const std::string raw=match[2];
        std::istringstream read(raw);read.imbue(std::locale::classic());
        double value;
        if (!(read>>value) || !std::isfinite(value)) throw GrammarImportError(number,"Invalid numeric value: "+raw);
        if (!item.overrides.emplace(match[1].str(),value).second)
            throw GrammarImportError(number,"Repeated parameter: "+match[1].str());
        found=true;
    }
    if(!found) throw GrammarImportError(number,"Expected a parameter after 'with'.");
    return item;
}

std::string formatNumber(double value) {
    char buffer[64];
    const auto result=std::to_chars(buffer,buffer+sizeof(buffer),value);
    if(result.ec!=std::errc()) throw std::runtime_error("Cannot format grammar parameter");
    return std::string(buffer,result.ptr);
}

std::string arguments(sqlite3* db,CatalogueImport item) {
    auto query=prepare(db,
        "SELECT p.name,p.default_value,p.minimum,p.maximum,p.unit,EXISTS("
        "SELECT 1 FROM properties x WHERE x.ref=p.ref AND x.name='integer:'||p.name) "
        "FROM parameters p WHERE p.ref=? ORDER BY p.ordinal",&item.reference);
    std::string args;
    while(nextRow(query.get())){
        auto row=query.get();
        const std::string name=column(row,0);
        const auto it=item.overrides.find(name);
        const double value=it==item.overrides.end()?sqlite3_column_double(row,1):it->second;
        const double lo=sqlite3_column_double(row,2),hi=sqlite3_column_double(row,3);
        if(!std::isfinite(value)||value<lo||value>hi||(sqlite3_column_int(row,5)&&value!=std::floor(value)))
            throw GrammarImportError(item.line,name+" requires "+(sqlite3_column_int(row,5)?"a whole number":"a number")+
               " from "+formatNumber(lo)+" to "+formatNumber(hi)+" "+column(row,4));
        args+=" "+formatNumber(value);
        if(it!=item.overrides.end()) item.overrides.erase(it);
    }
    if(!item.overrides.empty()) throw GrammarImportError(item.line,"Unknown parameter "+item.overrides.begin()->first);
    return args.empty()?"":" ("+args+" )";
}

std::string qualify(const std::string& line,const std::string& prefix) {
    static const std::regex symbol(R"(\b[RG][0-9a-f]{20}[A-Za-z0-9_]*\b)");
    return std::regex_replace(line,symbol,prefix+"$&");
}

std::string lower(std::string text) {
    for(char& ch:text) ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return text;
}
}

bool GrammarReuseCatalogue::isImportLine(const std::string& line) {
    const auto first=line.find_first_not_of(" \t\r");
    if(first==std::string::npos || line.compare(first,4,"@use")!=0) return false;
    return first+4==line.size() || (!std::isalnum(static_cast<unsigned char>(line[first+4])) && line[first+4]!='_');
}

std::vector<GrammarReuseListing> GrammarReuseCatalogue::listEntries() {
    auto db=openDatabase();
    auto query=prepare(db.get(),"SELECT ref,title,category,search_text FROM catalogue ORDER BY title,ref");
    std::vector<GrammarReuseListing> result;
    while(nextRow(query.get())) result.push_back({column(query.get(),0),column(query.get(),1),column(query.get(),2),column(query.get(),3)});
    return result;
}

std::string GrammarReuseCatalogue::readStandalone(const std::string& reference) {
    auto db=openDatabase();
    return readGrammar(db.get(),reference);
}

std::optional<ExpandedGrammarSource> GrammarReuseCatalogue::expandImports(const std::vector<std::string>& lines) {
    if(std::none_of(lines.begin(),lines.end(),isImportLine)) return std::nullopt;
    static const std::regex localRule(R"(^\s*([A-Za-z_]\w*)\s*(?:\(|\s|->))");
    static const std::regex startRule(R"(^Start -> ([A-Za-z_]\w*)(?:\s*\([^()]*\))?\s*$)");
    static const std::regex reservedPrefix(R"(^Dbr[0-9]+X)");
    const std::set<std::string> reserved{"Start","Body","T","S","A","R","I","G","V","M","P","X","Y","Z","Object","Interface","Connect","Joint","Motor","Switch","Repeat","Light","Material","Cube","Sphere","Cylinder","Grid","Split","Time","rgb","material","t","seed"};
    constexpr size_t limit=2*1024*1024;
    ExpandedGrammarSource expanded{lines,{}};
    std::vector<CatalogueImport> imports;
    std::set<std::string> declared,local;
    std::string authored;
    for(size_t i=0;i<lines.size();++i){
        if(authored.size()+lines[i].size()+1>limit) throw GrammarImportError(i,"Grammar exceeds the 2 MiB import limit.");
        authored+=lines[i]+"\n";
        if(isImportLine(lines[i])){
            auto item=parseImport(lines[i],i);
            if(reserved.count(item.alias)||std::regex_search(item.alias,reservedPrefix)||!declared.insert(item.alias).second)
                throw GrammarImportError(i,"Duplicate or reserved reuse alias "+item.alias);
            imports.push_back(std::move(item));expanded.lines[i].clear();
        }else{
            const std::string visible=lines[i].substr(0,std::min(lines[i].find('#'),lines[i].find("//")));
            const auto start=visible.find_first_not_of(" \t\r");
            if(start!=std::string::npos&&visible[start]=='@') throw GrammarImportError(i,"Database imports require native grammar rules.");
            std::smatch head;
            if(visible.find("->")!=std::string::npos && std::regex_search(visible,head,localRule))
                local.insert(head[1].str());
        }
    }
    if(local.empty())throw GrammarImportError(imports.front().line,"Add a start rule which uses the imported alias.");
    size_t size=authored.size();
    size_t serial=0;
    Database db(nullptr,sqlite3_close);
    for(const auto& item:imports){
        try {
            if(local.count(item.alias)) throw GrammarImportError(item.line,"Local rule already uses "+item.alias);
            if(!db) db=openDatabase();
            const std::string grammar=readGrammar(db.get(),item.reference);
            std::istringstream input(grammar);
            std::string line,prefix;std::smatch head;
            if(!std::getline(input,line)||!std::regex_match(line,head,startRule))
                throw GrammarImportError(item.line,"Unsupported catalogue start rule");
            do {prefix="Dbr"+std::to_string(++serial)+"X";} while(authored.find(prefix)!=std::string::npos);
            auto append=[&](std::string value) {
                size+=value.size()+1;
                if(size>limit)throw GrammarImportError(item.line,"Expanded grammar exceeds the 2 MiB import limit.");
                expanded.lines.push_back(std::move(value));expanded.declarationLines.push_back(item.line);
            };
            append(item.alias+" -> "+qualify(head[1].str(),prefix)+arguments(db.get(),item));
            while(std::getline(input,line)) append(qualify(line,prefix));
        } catch(const GrammarImportError&){throw;}
          catch(const std::exception& e){throw GrammarImportError(item.line,e.what());}
    }
    return expanded;
}

std::optional<int> GrammarReuseCatalogue::runCommand(int argc,char** argv) {
    if(argc<2) return std::nullopt;
    const std::string action=argv[1];
    if(action!="--list-reuse"&&action!="--export-reuse"&&action!="--expand-reuse")return std::nullopt;
    try {
        if(action=="--list-reuse"&&argc<=3){
            const std::string query=argc==3?lower(argv[2]):"";
            for(const auto& e:listEntries())if(query.empty()||lower(e.searchText).find(query)!=std::string::npos)
                std::cout<<e.reference<<"\t"<<e.title<<"\n";
        }else if(action=="--export-reuse"&&argc==3) std::cout<<readStandalone(argv[2]);
        else if(action=="--expand-reuse"&&argc==3){
            std::ifstream in(argv[2]);if(!in)throw std::runtime_error("Cannot open source file "+std::string(argv[2]));
            std::string line;std::vector<std::string> lines;
            while(std::getline(in,line))lines.push_back(line);
            const auto expanded=expandImports(lines);
            for(const auto& result:expanded?expanded->lines:lines)std::cout<<result<<"\n";
        }else throw std::runtime_error("Use --list-reuse [text], --export-reuse REF, or --expand-reuse FILE.");
        return 0;
    }catch(const GrammarImportError& e){std::cerr<<"Line "<<e.line+1<<": "<<e.what()<<"\n";return 1;}
     catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
