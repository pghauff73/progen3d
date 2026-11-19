#include "Context.h"
#include "GrammarRuntime.h"
#include "editor/service/GrammarCompilationService.h"
#include "grammar.h"
#include "physics/model/EngineeringDevicePhysics.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
using Parameters = EngineeringDevicePhysics::Parameters;

void expect(bool ok, const char* detail)
{
    if (!ok) throw std::runtime_error(detail);
}

void near(double lhs, double rhs, double tolerance, const char* detail)
{
    expect(std::isfinite(lhs) && std::abs(lhs-rhs) <= tolerance, detail);
}

double at(double time, const char* name, const Parameters& parameters)
{
    return EngineeringDevicePhysics(time).bind(name, parameters);
}

void checkThermalHysteresis()
{
    Parameters p{293.15,10,2,.1,1,315.15,308.15};
    // Independent explicit heat balance over repeated close/reopen events.
    double temperature = p[0];
    bool closed = false;
    for (int step = 0; step <= 200000; ++step) {
        const double t = step*.0001;
        if (step % 10000 == 0) {
            near(at(t,"thermalKelvin",p), temperature, .004, "temperature vs energy balance");
            near(at(t,"thermalClosed",p), closed, 0, "thermal latch sequence");
        }
        temperature += .0001*(p[1]-(closed?p[4]:p[3])*(temperature-p[0]))/p[2];
        if (temperature >= p[5]) closed = true;
        if (temperature <= p[6]) closed = false;
    }
    p[1] = 1;
    near(at(100,"thermalClosed",p),0,0,"subthreshold input");
    p[1] = 45;
    near(at(100,"thermalClosed",p),1,0,"latched hot contact");
}

void checkWaterBalance()
{
    Parameters p{43200,43200,18000,16000,.14,.8,7200};
    const double wet = at(6,"gelMass",p), dry = at(12,"gelMass",p);
    near(at(0,"gelMass",p),dry,1e-12,"periodic moisture inventory");
    near(at(6,"collectedMass",p),0,0,"collection during uptake");
    near(at(12,"collectedMass",p),.8*(wet-dry),1e-12,"released and collected masses");
    near(at(12000,"collectedMass",p),1000*.8*(wet-dry),1e-9,"accumulated cycle mass");
    near((at(3.001,"gelMass",p)-at(2.999,"gelMass",p))/.002,
         (.14-at(3,"gelMass",p))*7200/18000,1e-8,"sorption rate");
    p[5]=0;
    near(at(37,"collectedMass",p),0,0,"zero collection efficiency");
    p[4]=0;
    near(at(23,"gelMass",p),0,0,"zero gel capacity");
}

void checkCrawler()
{
    constexpr double pi=3.14159265358979323846;
    Parameters p{0,8,.048,6,.45,22000,.00006,120,.55};
    const double omega=2*pi/p[3], lag=std::atan(omega*p[4]);
    const double attenuated=p[5]/std::hypot(1.0,omega*p[4]);
    double forceSum=0;
    constexpr double t=2, dt=.00001;
    for (int i=0;i<8;++i) {
        p[0]=i;
        const double phase=omega*t-2*pi*i/8-lag;
        const double velocity=(at(t+dt,"crawlerStation",p)-at(t-dt,"crawlerStation",p))/(2*dt);
        forceSum+=(1-p[8]*std::sin(phase))*velocity;
        const double start=at(t,"crawlerStation",p);
        p[0]=i+1;
        const double length=at(t,"crawlerStation",p)-start;
        near(p[7]*(p[2]-length),p[6]*attenuated*std::sin(phase-pi/8),1e-11,"pneumatic and spring forces");
    }
    near(forceSum,0,1e-9,"longitudinal force balance");
    p[0]=0;
    const double drift=at(6,"crawlerStation",p);
    expect(drift>0,"forward travel");
    p[8]=-.55;
    near(at(6,"crawlerStation",p),-drift,1e-12,"reverse travel");
    p[8]=0;
    near(at(6,"crawlerStation",p),0,1e-12,"isotropic drag");
    p[5]=0;
    p[0]=3;
    near(at(9,"crawlerStation",p),3*p[2],1e-12,"zero pressure amplitude");
}

void checkLattice()
{
    for (double damping : {0.,.4,2.,2.00000001,4.,100.}) {
        Parameters p{.4,.4,1,damping,1,.35};
        constexpr double dt=.00005;
        double theta=p[0], velocity=0;
        for (int i=0;i<80000;++i) {
            velocity+=dt*(p[5]-damping*velocity-(theta-p[1]));
            theta+=dt*velocity;
        }
        near(at(4,"latticeAngle",p),theta,2e-5,"angular momentum balance across damping regimes");
    }
}

void checkSceneIntegration(const std::string& source)
{
    GrammarCompilationService compiler({},{});
    auto compile=[&](const std::string& text,double t) {
        SceneGenerationRequest request;
        request.source_text=text;
        request.evaluation_time=t;
        return compiler.compile(request);
    };
    auto initial=compile(source,0);
    if (!initial->succeeded()) {
        for (const auto& issue:initial->diagnostics) std::cerr<<issue.message<<'\n';
        throw std::runtime_error("device example did not compile");
    }
    auto& scene=*initial->scene_snapshot->generationContext();
    auto readings0=scene.devicePhysics().readings();
    expect(readings0.size()==14,"all six quantities and nine distinct stations are bound");
    for(int i=0;i<96;++i) scene.stepSimulation(.0625f);
    auto stepped=scene.devicePhysics().readings();
    auto seek=compile(source,6);
    expect(seek->succeeded(),"random access native generation");
    expect(stepped==seek->scene_snapshot->generationContext()->devicePhysics().readings(),"step/seek agreement");
    expect(stepped!=readings0,"device physics advanced");
    scene.resetSimulation();
    expect(scene.devicePhysics().readings()==readings0,"reset restores initial physical state");
    auto& resumed=*seek->scene_snapshot->generationContext();
    resumed.stepSimulation(3);
    resumed.resetSimulation();
    near(resumed.devicePhysics().timeSeconds(),6,0,"reset to nonzero snapshot epoch");
    // Prove that the accepted mesh uses the thermal signal, rather than just
    // exposing a changing scalar in the inspector.
    std::string switchSource = source;
    const std::string selector = "Start -> Study(0)";
    switchSource.replace(switchSource.find(selector), selector.size(), "Start -> Study(1)");
    for (double t : {0., 6.}) {
        auto frame = compile(switchSource, t);
        expect(frame->succeeded(), "thermal geometry export");
        const auto& world = *frame->scene_snapshot->generationContext();
        std::vector<PrimitiveBounds> contacts;
        for (const auto& part : world.primitive_instances) {
            if (part.material_name == "copperbrushedmetal")
                contacts.push_back(world.getInstanceBounds(part));
        }
        expect(contacts.size() == 2, "thermal contact pair");
        near(contacts[1].min.y - contacts[0].max.y, t == 0 ? .55 : 0, 2.e-5,
             "gap measured between the generated solid faces");
    }
    auto simple=compile("Start -> I(Cube material(whitepaintmatte) 1 1)",0);
    expect(simple->succeeded() && simple->scene_snapshot->generationContext()->devicePhysics().empty(),"legacy grammar isolation");

    GrammarDocument probe;
    probe.loadFromSourceText("Start -> T(0 latticeAngle(0,0,1,1,1,1) 0) I(Cube material(whitepaintmatte) 1 1)","implicit-clock");
    expect(probe.error_details.empty() && grammarUsesTime(&probe),"implicit device clock triggers playback");
    probe.loadFromSourceText("Start -> T(0 1 0) I(Cube material(whitepaintmatte) 1 1)","static");
    expect(probe.error_details.empty() && !grammarUsesTime(&probe),"clock is cleared on source edit");
    for (const char* expression:{"latticeAngle(0,0,0,1,1,1)","thermalClosed(293,1,1,1,2,300,301)",
                                 "gelMass(1,1,1,1,1,2,1)","crawlerStation(1.5,8,1,1,1,1,1,1,1)",
                                 "collectedMass(1)","crawlerStation(0,8,1,1,0,8,1,1,0)"}) {
        auto rejected=compile(std::string("Start -> T(0 ")+expression+" 0) I(Cube material(whitepaintmatte) 1 1)",0);
        expect(!rejected->succeeded() && !rejected->diagnostics.empty(),"invalid model must produce a grammar diagnostic");
    }
}
}

int main(int argc,char** argv)
{
    try {
        if (argc!=2) throw std::runtime_error("provide the device example .p3d path");
        std::ifstream input(argv[1]);
        if (!input) throw std::runtime_error("cannot read example");
        const std::string source{std::istreambuf_iterator<char>(input),{}};
        checkThermalHysteresis();
        checkWaterBalance();
        checkCrawler();
        checkLattice();
        checkSceneIntegration(source);
        std::cout<<"Device physics: reference balances, native stepping, seeking, reset and grammar diagnostics passed.\n";
    } catch (const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}
