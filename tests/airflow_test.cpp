#include "Context.h"
#include "GrammarRuntime.h"
#include "grammar.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool condition, const std::string &message) {
    if (!condition) throw std::runtime_error(message);
}
void near(float value, float expected, float tolerance, const std::string &label) {
    require(std::isfinite(value) && std::abs(value-expected) <= tolerance,
            label + ": got " + std::to_string(value) + ", expected " + std::to_string(expected));
}
std::unique_ptr<GrammarDocument> scene(std::istream &input, const std::string &name, bool valid=true) {
    auto grammar = std::make_unique<GrammarDocument>();
    setGrammarEvaluationDiagnosticsQuiet(grammar.get(), true);
    grammar->loadFromStream(input, name);
    grammar->addContext();
    grammar->context->genPrimitives();
    grammar->generateGeometry();
    if (valid && !grammar->error_details.empty())
        throw std::runtime_error(name + ": " + grammar->error_details.front().message);
    if (!valid) {
        require(!grammar->error_details.empty(), name + " should fail");
        require(grammar->context->primitive_instances.empty(), name + " was partially executed");
    }
    return grammar;
}
std::unique_ptr<GrammarDocument> fixture(const std::string &actions, bool valid=true) {
    std::istringstream input("Start -> " + actions + "\n");
    return scene(input,"airflow fixture",valid);
}
std::unique_ptr<GrammarDocument> example(const std::string &name) {
    const auto path="examples/Airflow/"+name+".p3d";
    std::ifstream input(path);
    require(bool(input),"missing "+path);
    return scene(input,path);
}
const std::string cube=" I(Cube material(tealmatteceramic) 1 1)";
const std::string inertia=" M(2) BodyInertia(0.10 0.12 0.12) ";
void checkMechanics() {
    const float dt=.01f;
    for (const float offset : {0.f,.08f,-.08f}) {
        auto g=fixture(inertia+"LoadPoint(0 "+std::to_string(offset)+" 0) BodyForce(1.2 0 0)"+cube);
        auto &c=*g->context; auto &body=c.primitive_instances.at(0);
        body.sleeping=true; c.stepSimulation(dt);
        require(!body.sleeping,"load wakes sleeping body");
        near(body.velocity.x/dt,.6f,1e-6f,"forward acceleration");
        near(body.air_load.moment.z,-1.2f*offset,1e-6f,"yaw moment");
        near(glm::radians(body.rotational_velocity.z)/dt,-10*offset,1e-5f,"initial yaw acceleration");
        c.stepSimulation(dt);
        near(body.velocity.x,1.2f*dt,1e-6f,"no doubled accumulator");
        c.resetSimulation();
        near(body.velocity.x,0,0,"reset speed"); near(body.rotational_velocity.z,0,0,"reset rotation");
        c.stepSimulation(dt); near(body.velocity.x/dt,.6f,1e-6f,"replay");
    }
    auto g=fixture("A(90 2) "+inertia+"LoadPoint(0 0.08 0) BodyForce(1.2 0 0)"+cube);
    g->context->stepSimulation(dt);
    near(g->context->primitive_instances[0].air_load.force.y,1.2f,1e-5f,"rotated force");
    auto doubled=fixture("M(2) BodyInertia(0.18 0.18 0.24) LoadPoint(0 0.08 0) BodyForce(1.2 0 0)"+cube);
    doubled->context->stepSimulation(dt);
    near(glm::radians(doubled->context->primitive_instances[0].rotational_velocity.z)/dt,-.4f,1e-5f,"double inertia");
    auto couple=fixture(inertia+"LoadPoint(0 0.1 0) BodyForce(1 0 0) LoadPoint(0 -0.1 0) BodyForce(-1 0 0)"+cube);
    couple->context->stepSimulation(dt);
    const auto &b=couple->context->primitive_instances[0];
    near(glm::length(b.velocity),0,1e-7f,"couple translation"); near(b.air_load.moment.z,-.2f,1e-6f,"couple torque");
    auto legacy=fixture("G(2 0 -1 0) M(2)"+cube);
    legacy->context->stepSimulation(dt);
    near(legacy->context->primitive_instances[0].velocity.y,-.02f,1e-6f,"legacy gravity");
    auto bad=fixture(cube+" BodyInertia(1 1 9)"+cube,false);
    require(bad->error_details.front().line>=0,"source diagnostic");
    fixture(inertia+"PropSample(0 1 1)"+cube,false);
    fixture(inertia+"BodyForce(1 0 0)",false);
}

void checkReplayAndFreeRotation() {
    auto ramp=fixture(inertia+" BodyForce(2 0 0) ForceRamp(0 1 1)"+cube);
    auto &c=*ramp->context;
    for(int i=0;i<60;++i)c.stepSimulation(1.f/120);
    near(c.inspectAirflow(c.primitive_instances[0]).force.x,1.f,2e-5f,"ramp at half a second");
    c.resetSimulation(); near(c.inspectAirflow(c.primitive_instances[0]).force.x,0,1e-8f,"reset ramp");
    auto g=fixture("M(2) VR(3 7 11) BodyInertia(0.08 0.10 0.12)"+cube);
    auto &body=g->context->primitive_instances[0];
    const auto momentum=[&] {
        glm::mat3 basis(body.primary_transform);
        for(int col=0;col<3;++col) basis[col]=glm::normalize(basis[col]);
        return basis*(body.airflow->principalInertia*(glm::transpose(basis)*glm::radians(body.rotational_velocity)));
    };
    const auto initial=momentum();
    for(int i=0;i<120;++i)g->context->stepSimulation(1.f/120);
    near(glm::length(momentum()-initial),0,1e-5f,"torque-free angular momentum");
}
void checkAir() {
    auto wind=fixture(inertia+" Wind(4 0 0) V(4 0 0) DragSurface(1 0.2 1)"+cube);
    auto &c=*wind->context;auto &b=c.primitive_instances[0];
    near(glm::length(c.inspectAirflow(b).force),0,1e-7f,"matching wind");
    auto rotation=fixture(inertia+" VR(0 0 90) LoadPoint(1 0 0) DragSurface(1 0.2 1)"+cube);
    const auto &r=*rotation->context;
    require(r.inspectAirflow(r.primitive_instances[0]).moment.z<0,"rotational drag opposes spin");
    const float expected[]={.035856f,.0790313f,.1260653f};
    for(int i=0;i<3;++i) {
        auto g=fixture(inertia+" Wind(15 0 0) PlateFriction(0.2 1 "+std::to_string(i)+")"+cube);
        near(g->context->inspectAirflow(g->context->primitive_instances[0]).force.x,expected[i],1e-5f,"plate friction");
    }
}

void checkPropellerAndDuct() {
    auto cart=example("FanCart"); auto &c=*cart->context; auto &b=c.primitive_instances.at(0);
    const auto saved=b.initial_velocity;
    b.velocity.x=.421f*(5004.f/60)*.2159f;
    const auto sample=c.inspectAirflow(b);
    near(sample.thrust,1.28942f,.0001f,"thrust"); near(sample.shaftPower,16.3272f,.001f,"shaft power");
    near(sample.moment.x,-.031158f,.00002f,"shaft reaction"); near(sample.force.x,.25505f,.0001f,"net force");
    b.velocity.x=20;
    require(!c.inspectAirflow(b).withinRange,"map range");
    b.velocity=saved;
    const int steps=3000;
    for(int i=0;i<steps;++i)c.stepSimulation(.01f);
    near(b.velocity.x,8.154f,.02f,"30 s response"); const float finalSpeed=b.velocity.x;
    c.resetSimulation();
    for(int i=0;i<steps*2;++i)c.stepSimulation(.005f);
    near(b.velocity.x,finalSpeed,.01f,"time-step refinement");
    auto stand=example("DuctedFanStand");auto &assembly=*stand->context;
    require(assembly.primitive_instances.size()==27,"duct assembly part count");
    const auto point=assembly.primitive_instances.front().airflow->ductOperatingPoint();
    require(point.unique(),"unique operating point");near(point.flow,.4f,1e-6f,"flow");
    near(point.pressure,72,1e-5f,"pressure");near(point.flow*point.pressure,28.8f,.001f,"air power");
    BodyAirflow duct;
    duct.ductStations={{0,9,0},{1,0,3}};
    near(duct.ductOperatingPoint().flow,.75f,1e-6f,"interpolated intersection");
    duct.ductStations={{0,0,1},{1,1,3}};require(!duct.ductOperatingPoint().unique(),"no intersection");
    duct.ductStations={{0,1,0},{1,0,1},{2,3,1}};
    require(duct.ductOperatingPoint().intersections==2,"multiple intersections");
    duct.ductStations={{0,1,1},{1,2,2}};
    require(duct.ductOperatingPoint().coincidentInterval,"coincident interval");
    example("OffsetFan");
}
}
int main(int argc, char **argv) {
    try {
        if (argc == 4 && std::string(argv[1]) == "--trace") {
            std::ifstream source(argv[2]);
            require(bool(source), "cannot open grammar");
            // The legacy parser writes informational messages to stdout.
            auto *csv=std::cout.rdbuf(std::cerr.rdbuf());
            auto g=scene(source, argv[2]);
            std::cout.rdbuf(csv);
            std::size_t end=0;
            unsigned long steps=std::stoul(argv[3], &end);
            require(end==std::string(argv[3]).size() && steps<=7200,"expected 0..7200 ticks");
            auto &c=*g->context;
            std::cout<<"time,instance,fx,fy,fz,mx,my,mz,vx,vy,vz,thrust,power,shaftTorque,inRange\n";
            for(unsigned long k=0;k<=steps;++k) {
                for(std::size_t i=0;i<c.primitive_instances.size();++i) {
                    const auto &part=c.primitive_instances[i];
                    if(!part.airflow) continue;
                    const auto load=c.inspectAirflow(part);
                    std::cout<<std::setprecision(9)<<c.time<<','<<i;
                    for(auto v : {load.force,load.moment,part.velocity})
                        std::cout<<','<<v.x<<','<<v.y<<','<<v.z;
                    std::cout<<','<<load.thrust<<','<<load.shaftPower<<','<<load.shaftTorque<<','<<load.withinRange<<'\n';
                }
                if(k<steps)c.stepSimulation(1.f/120);
            }
            return 0;
        }
        require(argc==1, "usage: airflow-test [--trace FILE TICKS]");
        checkMechanics(); checkAir(); checkReplayAndFreeRotation(); checkPropellerAndDuct();
        std::cout<<"Native airflow grammar, rigid-body, propeller, duct and plate checks passed.\n";
    } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
