#pragma once
#include "EnergyCatalog.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialInterfaceType.h"
#include "spatial/model/SpatialInterfaceState.h"
namespace epg {
inline bool matches_native_type(const std::string& domain,SpatialInterfaceType t){
 if(domain=="electrical")return t==SpatialInterfaceType::ElectricalPort;
 if(domain=="gas"||domain=="hydraulic"||domain=="chemical")return t==SpatialInterfaceType::PipePort;
 if(domain=="shaft")return t==SpatialInterfaceType::Shaft;
 if(domain=="thermal")return t==SpatialInterfaceType::ThermalInterface;
 if(domain=="control")return t==SpatialInterfaceType::ControlPort;
 return (domain=="radiant"||domain=="microwave")&&t==SpatialInterfaceType::ThermalInterface;
}
inline Result check_spatial_model(const SpatialBuildingModel& model){
 const auto cat=default_catalog();std::vector<Object> objects;std::vector<Link> links;
 std::set<Endpoint> energy_ports;
 for(const auto& o:model.objects().objects()){
  auto cls=o.identity().objectClass().value();auto k=cat.find(cls);if(k==cat.end())continue;
  Object x{o.identity().objectId().value(),cls,{}};
  for(const auto& p:o.interfaces()){
   auto i=k->second.ports.find(p.interfaceId().value());
   if(i!=k->second.ports.end()&&matches_native_type(i->second.domain,p.type())&&p.state()!=SpatialInterfaceState::Invalid){x.ports.insert(i->first);energy_ports.insert({x.id,i->first});}
  }
  objects.push_back(x);
 }
 for(const auto& c:model.connectionGraph().connections()){
  if(c.state()==SpatialConnectionState::Broken||c.state()==SpatialConnectionState::Invalid)continue;
  if(c.connectionType()!=SpatialConnectionType::Powers&&c.connectionType()!=SpatialConnectionType::ConnectedTo)continue;
  Endpoint a{c.sourceInterface().objectId().value(),c.sourceInterface().interfaceId().value()};
  Endpoint b{c.targetInterface().objectId().value(),c.targetInterface().interfaceId().value()};
  if(!energy_ports.count(a)&&!energy_ports.count(b))continue;
  links.push_back({c.connectionId().value(),a,b});
 }
 return check(cat,objects,links);
}
}
