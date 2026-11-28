#pragma once
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>
namespace epg {
struct Port {std::string domain; bool input; bool optional;};
enum class InputRequirement { AllRequired, AnyDeclared };
struct Model {
	std::string role;
	bool established;
	std::map<std::string, Port> ports;
	InputRequirement input_requirement = InputRequirement::AllRequired;
};
using Catalog=std::map<std::string,Model>;
using Endpoint=std::pair<std::string,std::string>;
struct Object {
	std::string id, cls;
	std::set<std::string> ports;
	std::optional<Model> model_override = std::nullopt;
};
struct Link {std::string id; Endpoint from,to;};
struct Warning {std::string code,object,message;};
struct Result {std::vector<Warning> warnings; std::map<Endpoint,int> quality;};
// 0 = no supply, 1 = research/speculative only, 2 = established model path.
// Monotone propagation from declared sources; cycles cannot create supply.
inline Result check(const Catalog& cat,const std::vector<Object>& objects,const std::vector<Link>& links) {
 Result r; std::map<std::string,const Object*> byid;
 auto warn=[&](const std::string& code,const std::string& id,const std::string& text){r.warnings.push_back({code,id,text});};
 for(const auto& o:objects) if(!byid.emplace(o.id,&o).second) warn("EPG005",o.id,"Duplicate object identifier.");
 auto model=[&](const Object& o)->const Model* {
   if(o.model_override)return &*o.model_override;
   auto m=cat.find(o.cls);return m==cat.end()?nullptr:&m->second;
 };
 auto port=[&](const Endpoint& e)->const Port* {auto i=byid.find(e.first); if(i==byid.end()||!i->second->ports.count(e.second)) return nullptr;auto m=model(*i->second); if(!m)return nullptr;auto p=m->ports.find(e.second);return p==m->ports.end()?nullptr:&p->second;};
 std::vector<Link> valid; std::set<std::string> linkids;
 for(const auto& l:links){
   if(!linkids.insert(l.id).second){warn("EPG005",l.id,"Duplicate connection identifier.");continue;}
   auto a=port(l.from),b=port(l.to);
   if(!a||!b){warn("EPG004",l.id,"Unresolved or unrecognised energy port; link ignored.");continue;}
   if(a->input||!b->input||a->domain!=b->domain){warn("EPG003",l.id,"Incompatible energy domain or port direction; link ignored.");continue;}
   valid.push_back(l);
 }
 for(const auto& o:objects){auto m=model(o); if(!m)continue; for(const auto& p:m->ports)if(!p.second.input&&o.ports.count(p.first)&&m->role=="source")r.quality[{o.id,p.first}]=m->established?2:1;}
 bool changed=true;
 auto raise=[&](const Endpoint& e,int v){int& old=r.quality[e];if(v>old){old=v;changed=true;}};
 while(changed){changed=false;
   for(const auto& l:valid)raise(l.to,r.quality[l.from]);
   for(const auto& o:objects){auto m=model(o); if(!m||!(m->role=="transfer"||m->role=="converter"||m->role=="motor"))continue;
     int q=m->established?2:1;
     if(m->input_requirement==InputRequirement::AnyDeclared){
       int best_input=0;
       for(const auto& p:m->ports)if(p.second.input&&!p.second.optional&&o.ports.count(p.first)){
         best_input=std::max(best_input,r.quality[{o.id,p.first}]);
       }
       q=std::min(q,best_input);
     }else{
       bool hasRequired=false;
       for(const auto& p:m->ports)if(p.second.input&&!p.second.optional){hasRequired=true;q=std::min(q,o.ports.count(p.first)?r.quality[{o.id,p.first}]:0);}
       if(!hasRequired)continue;
     }
     for(const auto& p:m->ports)if(!p.second.input&&o.ports.count(p.first))raise({o.id,p.first},q);
   }
 }
 for(const auto& o:objects){auto m=model(o);if(!m||m->role!="motor")continue;
   for(const auto& p:m->ports)if(p.second.input&&!p.second.optional){
     Endpoint e{o.id,p.first};int q=o.ports.count(p.first)?r.quality[e]:0;
     if(q==0)warn("EPG001",o.id,"Motor input '"+p.first+"' has no reachable compatible power source.");
     else if(q==1)warn("EPG002",o.id,"Motor input '"+p.first+"' is supplied only by research/speculative power models.");
   }
 }
 return r;
}
}
