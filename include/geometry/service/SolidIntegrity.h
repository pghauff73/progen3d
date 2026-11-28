#pragma once
// SIC v1: dependency-free C++17, world-space geometry (metres).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <vector>
namespace sic {
struct Vec { double x=0,y=0,z=0; double operator[](int i) const {return i==0?x:i==1?y:z;} };
inline Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec operator*(Vec a,double s){return {a.x*s,a.y*s,a.z*s};}
inline double dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double norm(Vec a){return std::sqrt(dot(a,a));}
inline Vec unit(Vec a){return a*(1/norm(a));}
inline bool finite(Vec a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
using Tri=std::array<int,3>;
struct Mesh { std::string id; std::vector<Vec> vertices; std::vector<Tri> faces; bool assigned=false; int line=-1, column=0, end_column=0; };
struct Settings { double area_ratio=.02, normal_difference=.01, plane_slope=.005, plane_gap_relative=.005, touch_relative=1e-6, weld_relative=1e-8, patch_relative=1e-6; std::uint64_t work_limit=5000000; };
struct Diagnostic { std::string code,id,message; bool error; int line,column,end_column; };
struct Contact { std::string a,b; double area=0,ratio=0,normal_difference=0,slope=0,max_gap=0,min_gap=0; bool qualifies=false; };
struct Report { std::vector<Diagnostic> diagnostics; std::vector<Contact> contacts; bool complete=true; std::size_t valid_meshes=0; bool succeeded() const {return std::none_of(diagnostics.begin(),diagnostics.end(),[](const Diagnostic &d){return d.error;});} };
struct Box { Vec lo,hi; };
inline Box bounds(const std::vector<Vec>& v){Box b{v.front(),v.front()};for(auto p:v){b.lo={std::min(b.lo.x,p.x),std::min(b.lo.y,p.y),std::min(b.lo.z,p.z)};b.hi={std::max(b.hi.x,p.x),std::max(b.hi.y,p.y),std::max(b.hi.z,p.z)};}return b;}
inline bool overlap(Box a,Box b,double g){for(int i=0;i<3;i++)if(a.hi[i]+g<b.lo[i]||b.hi[i]+g<a.lo[i])return false;return true;}
inline void diag(Report& r,const Mesh& m,const char* code,bool e,const std::string& msg){r.diagnostics.push_back({code,m.id,msg,e,m.line,m.column,m.end_column});}
struct Face { Tri ids; Vec normal; double area; };
struct Patch { Vec n,p; std::vector<int> faces; Box box; };
struct Solid { bool valid=false; double area=0,scale=0; Box box{}; std::vector<Vec> v; std::vector<Face> f; std::vector<Patch> patches; };
inline Solid prepare(const Mesh& m,const Settings& s,Report& r){
 Solid o; if(m.vertices.empty()||m.faces.empty()){diag(r,m,"SIC003",true,"Empty solid mesh");return o;}
 if(std::any_of(m.vertices.begin(),m.vertices.end(),[](Vec v){return !finite(v);})){diag(r,m,"SIC003",true,"Nonfinite vertex");return o;}
 o.box=bounds(m.vertices);o.scale=norm(o.box.hi-o.box.lo);if(!(o.scale>0&&std::isfinite(o.scale))){diag(r,m,"SIC003",true,"Zero or invalid mesh extent");return o;}
 const double weld=o.scale*s.weld_relative, plane=o.scale*s.patch_relative;
 // Local-origin quantisation; never weld across different instances.
 using Key=std::tuple<long long,long long,long long>;std::map<Key,int> keys;std::vector<int> remap;
 for(auto p:m.vertices){Vec q=(p-o.box.lo)*(1/weld);Key k{std::llround(q.x),std::llround(q.y),std::llround(q.z)};auto it=keys.emplace(k,(int)o.v.size());if(it.second)o.v.push_back(p);remap.push_back(it.first->second);}
 using Edge=std::pair<int,int>;std::map<Edge,std::vector<std::pair<int,int>>> edges; std::set<Tri> unique; std::size_t deg=0;
 for(auto t:m.faces){bool bad=false;for(int i:t)if(i<0||std::size_t(i)>=remap.size())bad=true;if(bad){deg++;continue;}for(int& i:t)i=remap[i];Tri sorted=t;std::sort(sorted.begin(),sorted.end());Vec c=cross(o.v[t[1]]-o.v[t[0]],o.v[t[2]]-o.v[t[0]]);double a=norm(c)*.5;if(sorted[0]==sorted[1]||sorted[1]==sorted[2]||!std::isfinite(a)||a<=weld*weld||!unique.insert(sorted).second){deg++;continue;}int fi=(int)o.f.size();o.f.push_back({t,unit(c),a});o.area+=a;for(int k=0;k<3;k++){int x=t[k],y=t[(k+1)%3];edges[std::minmax(x,y)].push_back({fi,x<y?1:-1});}}
 if(deg)diag(r,m,"SIC003",true,std::to_string(deg)+" invalid, duplicate or degenerate faces");
 std::size_t open=0,nonmanifold=0,winding=0;std::vector<std::vector<int>> adj(o.f.size());
 for(const auto& e:edges){const auto& f=e.second;if(f.size()==1)open++;else if(f.size()!=2)nonmanifold++;else{if(f[0].second==f[1].second)winding++;adj[f[0].first].push_back(f[1].first);adj[f[1].first].push_back(f[0].first);}}
 if(open)diag(r,m,"SIC001",true,std::to_string(open)+" boundary edges: open mesh");
 if(nonmanifold)diag(r,m,"SIC002",true,std::to_string(nonmanifold)+" non-manifold edges");
 if(winding)diag(r,m,"SIC003",true,std::to_string(winding)+" inconsistent-winding edges");
 if(deg||open||nonmanifold||winding||o.f.empty())return o;
 // Vertex link: incident triangle fan must be a single connected cycle.
 std::vector<std::vector<int>> incident(o.v.size());for(int i=0;i<(int)o.f.size();i++)for(int v:o.f[i].ids)incident[v].push_back(i);
 for(int v=0;v<(int)incident.size();v++){const auto& fan=incident[v];if(fan.empty())continue;std::set<int> seen{fan.front()};std::queue<int> q;q.push(fan.front());while(!q.empty()){int i=q.front();q.pop();for(int j:adj[i]){const auto& t=o.f[j].ids;if(std::find(t.begin(),t.end(),v)!=t.end()&&seen.insert(j).second)q.push(j);}}if(seen.size()!=fan.size()){diag(r,m,"SIC002",true,"Disconnected face fan at welded vertex");return o;}}
 // Each edge-connected shell must enclose nonzero signed volume. Preserve nested cavity winding.
 std::vector<char> visited(o.f.size());double total=0;for(int i=0;i<(int)o.f.size();i++)if(!visited[i]){double vol=0;Vec origin=o.v[o.f[i].ids[0]];std::queue<int> q;q.push(i);visited[i]=1;while(!q.empty()){int k=q.front();q.pop();auto t=o.f[k].ids;vol+=dot(o.v[t[0]]-origin,cross(o.v[t[1]]-origin,o.v[t[2]]-origin))/6;for(int j:adj[k])if(!visited[j]){visited[j]=1;q.push(j);}}if(std::abs(vol)<=weld*weld*weld){diag(r,m,"SIC003",true,"Closed shell has zero volume");return o;}total+=vol;}
 if(!(std::abs(total)>weld*weld*weld)){diag(r,m,"SIC003",true,"Zero net solid volume");return o;}if(total<0)for(auto& f:o.f)f.normal=f.normal*(-1);
 // Grow connected coplanar patches against a fixed seed plane, not a drifting local normal.
 std::fill(visited.begin(),visited.end(),0);for(int i=0;i<(int)o.f.size();i++)if(!visited[i]){Patch p{o.f[i].normal,o.v[o.f[i].ids[0]],{}, {}};std::vector<Vec> pv;std::queue<int> q;q.push(i);visited[i]=1;while(!q.empty()){int k=q.front();q.pop();p.faces.push_back(k);for(int v:o.f[k].ids)pv.push_back(o.v[v]);for(int j:adj[k])if(!visited[j]){auto f=o.f[j];bool ok=norm(f.normal-p.n)<=1e-5;for(int v:f.ids)ok=ok&&std::abs(dot(p.n,o.v[v]-p.p))<=plane;if(ok){visited[j]=1;q.push(j);}}}p.box=bounds(pv);o.patches.push_back(std::move(p));}
 o.valid=true;return o;
}
struct P2 { double x,y; };inline P2 operator+(P2 a,P2 b){return {a.x+b.x,a.y+b.y};}inline P2 operator-(P2 a,P2 b){return {a.x-b.x,a.y-b.y};}inline P2 operator*(P2 a,double s){return {a.x*s,a.y*s};}inline double cross2(P2 a,P2 b){return a.x*b.y-a.y*b.x;}
inline double area2(const std::vector<P2>& p){double a=0;for(std::size_t i=0;i<p.size();i++)a+=cross2(p[i],p[(i+1)%p.size()]);return .5*a;}
inline std::vector<P2> clip(std::vector<P2> subject,std::vector<P2> cutter,double eps){if(area2(cutter)<0)std::reverse(cutter.begin(),cutter.end());for(int k=0;k<3&&!subject.empty();k++){P2 a=cutter[k],e=cutter[(k+1)%3]-a;std::vector<P2> next;for(std::size_t j=0;j<subject.size();j++){P2 x=subject[j],y=subject[(j+1)%subject.size()];double dx=cross2(e,x-a),dy=cross2(e,y-a);bool ix=dx>=-eps,iy=dy>=-eps;if(ix)next.push_back(x);if(ix!=iy&&std::abs(dx-dy)>std::numeric_limits<double>::min())next.push_back(x+(y-x)*std::clamp(dx/(dx-dy),0.,1.));}subject.swap(next);}return subject;}
inline bool compare(const Solid& a,const Solid& b,const Settings& s,Contact& best,std::uint64_t& work){
 const double L=std::min(a.scale,b.scale), gap=L*s.plane_gap_relative,touch=L*s.touch_relative, eps=L*L*1e-14;
 for(const auto& x:a.patches)for(const auto& y:b.patches){if(++work>s.work_limit)return false;if(!overlap(x.box,y.box,gap))continue;double d=norm(x.n+y.n),cosine=-dot(x.n,y.n);if(!(d<s.normal_difference&&cosine>0))continue;double slope=norm(cross(x.n,y.n))/cosine;if(slope>s.plane_slope)continue;
 Vec n=unit(x.n-y.n),u=unit(cross(n,std::abs(n.x)<.8?Vec{1,0,0}:Vec{0,1,0})),v=cross(n,u),origin=x.p;
 auto project=[&](Vec p){p=p-origin;return P2{dot(p,u),dot(p,v)};};
 auto tri=[&](const Solid& m,int f){std::vector<P2> t;for(int i:m.f[f].ids)t.push_back(project(m.v[i]));return t;};
 double area=0,min_signed=std::numeric_limits<double>::infinity(),max_signed=-min_signed;
 for(int i:x.faces){auto ta=tri(a,i);for(int j:y.faces){if(++work>s.work_limit)return false;auto tb=tri(b,j);auto p=clip(ta,tb,eps);double ar=std::abs(area2(p));if(ar<=eps)continue;area+=ar;for(P2 z:p){Vec p0=origin+u*z.x+v*z.y;double da=dot(x.n,x.p-p0)/dot(x.n,n),db=dot(y.n,y.p-p0)/dot(y.n,n);double signed_gap=db-da;min_signed=std::min(min_signed,signed_gap);max_signed=std::max(max_signed,signed_gap);}}}
 if(area<=eps)continue;
 double gmax=std::max(std::abs(min_signed),std::abs(max_signed)),gmin=(min_signed<=0&&max_signed>=0)?0:std::min(std::abs(min_signed),std::abs(max_signed));if(gmax>gap||gmin>touch)continue;
 double ratio=area/((a.area+b.area)*.5);if(ratio>best.ratio){best.area=area;best.ratio=ratio;best.normal_difference=d;best.slope=slope;best.max_gap=gmax;best.min_gap=gmin;best.qualifies=ratio+1e-12>=s.area_ratio;}}
 return true;
}
inline Report validate(const std::vector<Mesh>& meshes,const Settings& s={}){Report r;std::vector<Solid> solids;std::vector<bool> attached(meshes.size());std::vector<double> best_ratios(meshes.size());for(const auto& m:meshes){solids.push_back(prepare(m,s,r));if(solids.back().valid)r.valid_meshes++;}std::uint64_t work=0;
 for(std::size_t i=0;i<meshes.size()&&r.complete;i++)for(std::size_t j=i+1;j<meshes.size();j++){if(++work>s.work_limit){r.complete=false;break;}auto& a=solids[i];auto& b=solids[j];if(!a.valid||!b.valid||!overlap(a.box,b.box,std::min(a.scale,b.scale)*s.plane_gap_relative))continue;Contact c{meshes[i].id,meshes[j].id};if(!compare(a,b,s,c,work)){r.complete=false;break;}if(c.area>0){r.contacts.push_back(c);best_ratios[i]=std::max(best_ratios[i],c.ratio);best_ratios[j]=std::max(best_ratios[j],c.ratio);}if(c.qualifies)attached[i]=attached[j]=true;}
 if(!r.complete)r.diagnostics.push_back({"SIC005","scene","Contact search budget exceeded; attachment results incomplete",false,-1,0,0});
 for(std::size_t i=0;i<meshes.size();i++){if(solids[i].valid&&!meshes[i].assigned&&!attached[i]&&r.complete)diag(r,meshes[i],"SIC004",false,"Unassigned closed mesh has no qualifying planar contact; best eligible overlap="+std::to_string(100*best_ratios[i])+"%, required="+std::to_string(100*s.area_ratio)+"%");}
 return r;}
} // namespace sic
