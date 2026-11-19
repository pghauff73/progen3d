#include "mechanics/service/PrinterResponse.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace fff;

namespace {
void check(bool value,const char *why) { if (!value) throw std::runtime_error(why); }
void near(double value,double expected,double tolerance=1e-8) {
    if (!std::isfinite(value) || std::abs(value-expected)>tolerance)
        throw std::runtime_error("actual "+std::to_string(value)+", expected "+std::to_string(expected));
}
void reversalAndSpurFit(const PrinterProgram &p) {
    BacklashContact contact;
    near(contact.transmit(1,.2),.9); near(contact.transmit(.9,.2),.9);
    near(contact.transmit(.8,.2),.9); near(contact.transmit(.6,.2),.7);
    near(contact.transmit(1.5,0),1.5);
    auto gear=p.gears; const double gap=gear.backlashMm();
    gear.centreOffsetMm+=.1;
    near(gear.backlashMm()-gap,.2*std::tan(20*std::acos(-1)/180));
    gear.wheelThinningMm+=.02;
    near(gear.backlashMm()-gap,.02+.2*std::tan(20*std::acos(-1)/180));
}
void beltAndStructure(const PrinterProgram &p) {
    auto model=p.xDrive; model.dragN=0; model.massKg=1; model.pretensionN=4;
    model.spanNPerMm=50; model.toothNPerMm=100; model.mountNPerMm=200; model.jumpN=10;
    BeltTransmission belt(model,0);
    const auto atRest=belt.transmit(0,0,0); near(atRest.tightN,4); near(atRest.slackN,4);
    const auto taut=belt.transmit(2,0,4000);
    near(taut.tightN,6);near(taut.slackN,2);near(taut.stretchMm,.04);
    near(taut.toothMm,.04);near(taut.shearMm,.02);
    auto wide=model; wide.shaftRadiusMm*=2;
    near(BeltTransmission(wide,0).transmit(2,0,4000).torsionMm*16,taut.torsionMm);
    const auto skip=belt.transmit(2,0,12000);
    near(skip.slackN,0);near(skip.tightN,12);near(skip.stretchMm,.16);near(skip.strandPosition,0);
    check(skip.jumps==1,"one skipped tooth");check(belt.transmit(3,0,12000).jumps==1,"load held");
    near(belt.transmit(3,0,0).strandPosition,1);
    const auto reverse=belt.transmit(3,0,-12000);
    check(reverse.jumps==2,"reverse slip");near(reverse.strandPosition,3);
    const auto guide=p.guides; near(guide.sagMm(guide.spanMm/2),0);
    const double I=std::acos(-1)*std::pow(guide.diameterMm,4)/64;
    near(guide.sagMm(0),guide.carriageKg*9.80665*std::pow(guide.spanMm,3)/(96*guide.modulusMPa*I));
    near(guide.sagMm(22),guide.sagMm(-22));
}
void pressureAndTraction() {
    const double pi=std::acos(-1);
    Hotend h{2,.5,2,100,1}; MeltChamber chamber(h);
    auto first=chamber.advance(2,1);
    near(chamber.pressureMPa(),4*(1-std::exp(-1.)));
    near(first.feedMm3,first.outMm3+.5*chamber.pressureMPa());
    near(first.slipMm3,0);
    auto decay=chamber.advance(0,1);
    near(chamber.pressureMPa(),4*(1-std::exp(-1.))*std::exp(-1.));
    check(decay.outMm3>0 && decay.feedMm3==0,"feed stop must still ooze");
    h.nipN=pi; MeltChamber capped(h);
    auto one=capped.advance(3,1);
    const double hit=std::log(6./5.);
    near(capped.pressureMPa(),1);near(one.slipMm3,2.5*(1-hit));
    near(one.outMm3+.5+one.slipMm3,3);
    h.nipN=0; MeltChamber noGrip(h);
    near(noGrip.advance(2,1).slipMm3,2);near(noGrip.pressureMPa(),0);
    h.nipN=100; h.complianceMm3PerMPa=0; MeltChamber rigid(h);
    near(rigid.advance(2,1).outMm3,2);near(rigid.advance(0,1).outMm3,0);
}
void cycleAndConvergence(const PrinterProgram &p) {
    PrinterResponse response(p), refined(p,.0025);
    const double stockArea=std::acos(-1)*std::pow(p.hotend.filamentDiameterMm/2,2);
    for (double t=0;t<=response.duration();t+=.0625) {
        auto v=[&](const std::string &n){return response.value("fff"+n,t);};
        near(v("HobFeed"),v("Feed")+v("Slip"),2e-9);
        near(v("Feed")*stockArea,v("Deposit")+v("Ooze")+v("Stored"),2e-9);
        check(v("XSlack")>=0 && v("YSlack")>=0 && v("Pressure")>=0,"unilateral response");
        near(v("Pressure"),refined.value("fffPressure",t),.002);
    }
    const auto &last=response.last();
    check(last.slip>0 && last.ooze>0 && last.deposited>0,"coupled case must exercise slip and discharge");
    near(last.deposited,refined.last().deposited,.001);
    near(last.expansion,p.thermal.lengthMm*p.thermal.expansionPerK*p.thermal.hotMinusReferenceK);
    near(response.value("fffX",3),refined.value("fffX",3),.0001);
    near(response.value("fffZ",1e6),last.nozzle.z);
    near(response.value("fffX",-3),p.home.x);
    auto reference=p;
    reference.gears.pitchErrorMm=reference.gears.centreOffsetMm=reference.gears.wheelThinningMm=reference.gears.pinionThinningMm=0;
    reference.hotend.complianceMm3PerMPa=0; reference.hotend.nipN=1e6;
    const auto ideal=PrinterResponse(reference).last();
    near(ideal.deposited,576*(.24*(.48-.24)+std::acos(-1)*.24*.24/4));
    near(ideal.ooze,0);near(ideal.slip,0);
    auto bad=p;bad.gears.centreOffsetMm=-1;
    bool rejected=false;
    try { PrinterResponse invalid(bad); } catch (const std::invalid_argument &) { rejected=true; }
    check(rejected,"interfering mesh must be rejected");
}
}
int main(int argc,char **argv) {
    if (argc!=2) return 2;
    try {
        std::ifstream file(argv[1]); std::vector<std::string> lines;
        for (std::string s;std::getline(file,s);) lines.push_back(s);
        int where=0;auto p=PrinterProgramReader::read(lines,where);
        reversalAndSpurFit(p);beltAndStructure(p);pressureAndTraction();cycleAndConvergence(p);
        std::cout<<"Passed backlash, tooth fit, slack and reversal, deformation, pressure/traction, conservation and timestep refinement.\n";
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
