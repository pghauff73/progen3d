#include "mechanics/service/PrinterResponse.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace fff {
namespace {
constexpr double pi = 3.14159265358979323846;
void require(bool condition, const std::string &message) {
    if (!condition) throw std::invalid_argument(message);
}
bool positive(double v) { return std::isfinite(v) && v > 0; }
bool nonnegative(double v) { return std::isfinite(v) && v >= 0; }
double area(double d) { return pi*d*d/4; }
void validateBelt(const BeltDrive &b) {
    require(positive(b.massKg) && nonnegative(b.dragN) && nonnegative(b.pretensionN)
        && positive(b.spanNPerMm) && positive(b.toothNPerMm) && positive(b.mountNPerMm)
        && positive(b.shaftRadiusMm) && positive(b.shaftLengthMm) && positive(b.shearMPa)
        && nonnegative(b.backlashMm) && positive(b.jumpN) && positive(b.pitchMm)
        && b.pulleyTeeth >= 3 && std::floor(b.pulleyTeeth)==b.pulleyTeeth, "invalid belt drive values");
}
void validate(const PrinterProgram &p, double step) {
    validateBelt(p.xDrive); validateBelt(p.yDrive);
    const auto &g=p.gears;
    require(positive(g.moduleMm) && g.pinionTeeth>=3 && g.wheelTeeth>=3
        && std::floor(g.pinionTeeth)==g.pinionTeeth && std::floor(g.wheelTeeth)==g.wheelTeeth
        && g.pressureAngleDeg>0 && g.pressureAngleDeg<45 && nonnegative(g.pinionThinningMm)
        && nonnegative(g.wheelThinningMm) && nonnegative(g.pitchErrorMm) && positive(g.hobRadiusMm)
        && nonnegative(g.backlashMm()) && g.backlashMm()<pi*g.moduleMm/2
        && g.pitchErrorMm<g.moduleMm/2, "invalid gear geometry, interference or excessive clearance");
    require(positive(p.guides.spanMm) && positive(p.guides.diameterMm) && positive(p.guides.modulusMPa)
        && nonnegative(p.guides.carriageKg) && nonnegative(p.thermal.lengthMm)
        && nonnegative(p.thermal.expansionPerK) && std::isfinite(p.thermal.hotMinusReferenceK)
        && nonnegative(p.nutBacklashMm), "invalid guide, thermal stack or screw nut");
    const auto &h=p.hotend;
    require(positive(h.filamentDiameterMm) && nonnegative(h.complianceMm3PerMPa)
        && positive(h.resistanceMPaSPerMm3) && nonnegative(h.nipN) && nonnegative(h.friction)
        && positive(p.heightMm) && p.widthMm>=p.heightMm, "invalid melt chamber or bead");
    auto inTravel = [&](Point q) { return std::isfinite(q.x) && std::isfinite(q.y) && std::isfinite(q.z)
        && std::abs(q.x)<=p.guides.spanMm/2; };
    require(inTravel(p.home) && !p.moves.empty() && positive(step), "invalid home, step or empty toolpath");
    double sampleCount=1;
    for (const auto &m:p.moves) {
        require(positive(m.seconds) && nonnegative(m.extrusionMultiplier) && inTravel(m.target), "invalid move time, extrusion or travel");
        sampleCount+=std::ceil(m.seconds/step);
    }
    require(sampleCount<=120001, "printer trajectory exceeds 120000 integration steps");
}
}

double BacklashContact::transmit(double input, double gap) {
    output_ = std::clamp(output_, input-gap/2, input+gap/2);
    return output_;
}
double SpurGearPair::backlashMm() const {
    return pinionThinningMm + wheelThinningMm + 2*centreOffsetMm*std::tan(pressureAngleDeg*pi/180);
}
double GuideShaftPair::sagMm(double x) const {
    const double a=std::clamp(x+spanMm/2,0.,spanMm), b=spanMm-a;
    const double inertia=pi*std::pow(diameterMm,4)/64;
    return (carriageKg*9.80665/2)*a*a*b*b/(3*modulusMPa*inertia*spanMm);
}
AxisResponse BeltTransmission::transmit(double command, double velocity, double acceleration) {
    const auto &b=model_;
    // Smooth bearing drag through zero speed; each taut span changes tension by k*u.
    const double force=b.massKg*acceleration/1000 + b.dragN*std::tanh(velocity/0.5);
    const double load=std::abs(force);
    const double stretch=std::copysign(load<=2*b.pretensionN ? load/(2*b.spanNPerMm)
                                        : (load-b.pretensionN)/b.spanNPerMm,force);
    const double tight=b.pretensionN+b.spanNPerMm*std::abs(stretch);
    const double slack=std::max(0.,b.pretensionN-b.spanNPerMm*std::abs(stretch));
    const bool unloaded=load<.8*b.jumpN || slack>1e-9;
    if (unloaded) latchedDirection_=0;
    const int direction=force>=0 ? 1 : -1;
    if (slack<1e-9 && load>=b.jumpN && latchedDirection_!=direction) {
        signedJumps_+=direction;
        ++events_;
        latchedDirection_=direction;
    }
    const double strand=command-signedJumps_*b.pitchMm;
    const double radius=b.pitchMm*b.pulleyTeeth/(2*pi);
    const double polarMoment=pi*std::pow(b.shaftRadiusMm,4)/2;
    const double twist=force*radius*radius*b.shaftLengthMm/(b.shearMPa*polarMoment);
    const double tooth=force/b.toothNPerMm, shear=force/b.mountNPerMm;
    return {contact_.transmit(strand,b.backlashMm)-stretch-tooth-shear-twist,
            strand,tight,slack,stretch,tooth,shear,twist,events_};
}

MeltIncrement MeltChamber::advance(double attempted, double dt) {
    require(nonnegative(attempted) && positive(dt), "invalid feed increment or duration");
    const double rate=attempted/dt, r=model_.resistanceMPaSPerMm3, c=model_.complianceMm3PerMPa;
    const double limit=model_.nipN*model_.friction/area(model_.filamentDiameterMm);
    if (c==0) {
        pressure_=std::min(rate*r,limit);
        const double out=dt*pressure_/r;
        return {out,out,attempted-out};
    }
    const double old=pressure_, equilibrium=rate*r, tau=r*c;
    // Exact constant-inflow update. Limit the input only after traction saturates.
    pressure_=old+(equilibrium-old)*(-std::expm1(-dt/tau));
    double slip=0;
    if (pressure_>limit && equilibrium>limit) {
        const double hit=tau*std::log1p((limit-old)/(equilibrium-limit));
        slip=(rate-limit/r)*std::max(0.,dt-hit);
        pressure_=limit;
    }
    const double accepted=attempted-slip;
    return {accepted,std::max(0.,accepted-c*(pressure_-old)),slip};
}

PrinterResponse::PrinterResponse(const PrinterProgram &p, double step) {
    validate(p,step);
    BeltTransmission xDrive(p.xDrive,p.home.x), yDrive(p.yDrive,p.home.y);
    BacklashContact nut(p.home.z), gear;
    MeltChamber chamber(p.hotend);
    PrinterSample state;
    state.command=p.home;
    state.x=xDrive.transmit(p.home.x,0,0); state.y=yDrive.transmit(p.home.y,0,0);
    state.lift=p.home.z; state.sag=p.guides.sagMm(p.home.x);
    state.expansion=p.thermal.lengthMm*p.thermal.expansionPerK*p.thermal.hotMinusReferenceK;
    state.nozzle={p.home.x,p.home.y,state.lift-state.sag-state.expansion};
    times_.push_back(0); samples_.push_back(state);
    const double stockArea=area(p.hotend.filamentDiameterMm);
    const double beadArea=p.heightMm*(p.widthMm-p.heightMm)+pi*p.heightMm*p.heightMm/4;
    for (const auto &move:p.moves) {
        const Point start=state.command;
        const Point delta{move.target.x-start.x,move.target.y-start.y,move.target.z-start.z};
        const double startTime=times_.back(), startFeed=state.commandFeed;
        const double volume=std::hypot(std::hypot(delta.x,delta.y),delta.z)*beadArea*move.extrusionMultiplier;
        const int steps=static_cast<int>(std::ceil(move.seconds/step));
        const double dt=move.seconds/steps;
        for (int i=1;i<=steps;i++) {
            const double u=double(i)/steps, fraction=u*u*(3-2*u);
            const double speed=6*u*(1-u)/move.seconds, accel=(6-12*u)/(move.seconds*move.seconds);
            state.command={start.x+delta.x*fraction,start.y+delta.y*fraction,start.z+delta.z*fraction};
            state.x=xDrive.transmit(state.command.x,delta.x*speed,delta.x*accel);
            state.y=yDrive.transmit(state.command.y,delta.y*speed,delta.y*accel);
            state.lift=nut.transmit(state.command.z,p.nutBacklashMm);
            state.sag=p.guides.sagMm(state.x.position);
            state.nozzle={state.x.position,state.y.position,state.lift-state.sag-state.expansion};
            state.commandFeed=startFeed+volume*fraction/stockArea;
            const auto &g=p.gears;
            const double wheelAngle=state.commandFeed/g.hobRadiusMm;
            const double pitchTravel=g.wheelRadiusMm()*wheelAngle + g.pitchErrorMm*std::sin(g.wheelTeeth*wheelAngle);
            const double hobFeed=gear.transmit(pitchTravel,g.backlashMm())*g.hobRadiusMm/g.wheelRadiusMm();
            const auto material=chamber.advance(std::max(0.,hobFeed-state.hobFeed)*stockArea,dt);
            state.hobFeed=hobFeed;
            state.acceptedFeed+=material.feedMm3/stockArea;
            state.slip+=material.slipMm3/stockArea;
            state.pressure=chamber.pressureMPa();
            state.stored=p.hotend.complianceMm3PerMPa*state.pressure;
            state.flow=state.pressure/p.hotend.resistanceMPaSPerMm3;
            if (volume>0) state.deposited+=material.outMm3;
            else state.ooze+=material.outMm3;
            times_.push_back(startTime+move.seconds*u);
            samples_.push_back(state);
        }
    }
    constants_={{"fffDuration",duration()}, {"fffModule",p.gears.moduleMm}, {"fffPinionTeeth",p.gears.pinionTeeth},
        {"fffWheelTeeth",p.gears.wheelTeeth},{"fffCentreOffset",p.gears.centreOffsetMm},{"fffHobRadius",p.gears.hobRadiusMm},
        {"fffPinionThin",p.gears.pinionThinningMm},{"fffWheelThin",p.gears.wheelThinningMm},{"fffAngle",p.gears.pressureAngleDeg},{"fffGearGap",p.gears.backlashMm()},{"fffFilament",p.hotend.filamentDiameterMm},{"fffLayer",p.heightMm},{"fffWidth",p.widthMm},
        {"fffXRev",p.xDrive.pitchMm*p.xDrive.pulleyTeeth},{"fffYRev",p.yDrive.pitchMm*p.yDrive.pulleyTeeth}};
}

namespace {
using Projection = double (*)(const PrinterSample &);
const std::map<std::string, Projection> channels = {
    {"fffX", [](const PrinterSample &s) -> double { return s.nozzle.x; }},
    {"fffY", [](const PrinterSample &s) -> double { return s.nozzle.y; }},
    {"fffZ", [](const PrinterSample &s) -> double { return s.nozzle.z; }},
    {"fffCommandX", [](const PrinterSample &s) -> double { return s.command.x; }},
    {"fffCommandY", [](const PrinterSample &s) -> double { return s.command.y; }},
    {"fffCommandZ", [](const PrinterSample &s) -> double { return s.command.z; }},
    {"fffLift", [](const PrinterSample &s) -> double { return s.lift; }},
    {"fffSag", [](const PrinterSample &s) -> double { return s.sag; }},
    {"fffExpansion", [](const PrinterSample &s) -> double { return s.expansion; }},
    {"fffCommandFeed", [](const PrinterSample &s) -> double { return s.commandFeed; }},
    {"fffHobFeed", [](const PrinterSample &s) -> double { return s.hobFeed; }},
    {"fffFeed", [](const PrinterSample &s) -> double { return s.acceptedFeed; }},
    {"fffSlip", [](const PrinterSample &s) -> double { return s.slip; }},
    {"fffPressure", [](const PrinterSample &s) -> double { return s.pressure; }},
    {"fffFlow", [](const PrinterSample &s) -> double { return s.flow; }},
    {"fffDeposit", [](const PrinterSample &s) -> double { return s.deposited; }},
    {"fffOoze", [](const PrinterSample &s) -> double { return s.ooze; }},
    {"fffStored", [](const PrinterSample &s) -> double { return s.stored; }},
    {"fffXStrand", [](const PrinterSample &s) -> double { return s.x.strandPosition; }},
    {"fffXTight", [](const PrinterSample &s) -> double { return s.x.tightN; }},
    {"fffXSlack", [](const PrinterSample &s) -> double { return s.x.slackN; }},
    {"fffXStretch", [](const PrinterSample &s) -> double { return s.x.stretchMm; }},
    {"fffXTooth", [](const PrinterSample &s) -> double { return s.x.toothMm; }},
    {"fffXShear", [](const PrinterSample &s) -> double { return s.x.shearMm; }},
    {"fffXTwist", [](const PrinterSample &s) -> double { return s.x.torsionMm; }},
    {"fffXJumps", [](const PrinterSample &s) -> double { return s.x.jumps; }},
    {"fffYStrand", [](const PrinterSample &s) -> double { return s.y.strandPosition; }},
    {"fffYTight", [](const PrinterSample &s) -> double { return s.y.tightN; }},
    {"fffYSlack", [](const PrinterSample &s) -> double { return s.y.slackN; }},
    {"fffYStretch", [](const PrinterSample &s) -> double { return s.y.stretchMm; }},
    {"fffYTooth", [](const PrinterSample &s) -> double { return s.y.toothMm; }},
    {"fffYShear", [](const PrinterSample &s) -> double { return s.y.shearMm; }},
    {"fffYTwist", [](const PrinterSample &s) -> double { return s.y.torsionMm; }},
    {"fffYJumps", [](const PrinterSample &s) -> double { return s.y.jumps; }},
};
const std::set<std::string> constantNames = {"fffDuration", "fffModule", "fffPinionTeeth", "fffWheelTeeth",
    "fffCentreOffset", "fffHobRadius", "fffGearGap", "fffFilament", "fffLayer", "fffWidth", "fffXRev", "fffYRev", "fffPinionThin", "fffWheelThin", "fffAngle"};
}
bool PrinterResponse::recognizes(const std::string &name) {
    return channels.count(name) || constantNames.count(name);
}
double PrinterResponse::value(const std::string &name, double time) const {
    require(std::isfinite(time), "invalid sample time");
    auto constant=constants_.find(name);
    if (constant!=constants_.end()) return constant->second;
    const auto project=channels.at(name);
    auto upper=std::lower_bound(times_.begin(),times_.end(),time);
    if (upper==times_.begin()) return project(samples_.front());
    if (upper==times_.end()) return project(samples_.back());
    const auto i=static_cast<size_t>(upper-times_.begin());
    const double pre=project(samples_[i-1]), post=project(samples_[i]);
    if (name=="fffXJumps" || name=="fffYJumps") return time==*upper ? post : pre;
    const double u=(time-times_[i-1])/(*upper-times_[i-1]);
    return pre+(post-pre)*u;
}
}
