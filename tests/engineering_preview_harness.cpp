#include "editor/controller/PreviewTimelineController.h"
#include "vegetation/service/VegetationMeasurementUnitNormalizationService.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
int main() {
    int passed = 0;
    auto check = [&](bool ok, const char* name) {
        if (!ok) throw std::runtime_error(name);
        std::cout << name << "\n";
        ++passed;
    };
    auto near = [](double a, double b) { return std::abs(a-b) < 1e-6; };
    VegetationMeasurementUnitNormalizationService units;
    using Kind = VegetationMeasurementQuantityKind;
    check(near(units.normalize(2,"g/cm3",Kind::MassDensityKilogramsPerCubicMetre).normalizedValue().value(),2000),"density g/cm3 to kg/m3");
    check(near(units.normalize(4,"MPa",Kind::PressurePascals).normalizedValue().value(),4000000),"pressure MPa to Pa");
    check(near(units.normalize(30,"mm",Kind::LengthMetres).normalizedValue().value(),0.03),"length mm to m");
    check(near(units.normalize(std::acos(-1.0),"rad",Kind::AngleDegrees).normalizedValue().value(),180),"angle rad to degrees");
    check(near(units.normalize(7,"percent",Kind::Fraction).normalizedValue().value(),.07),"percent to fraction");
    check(!units.normalize(1,"K",Kind::LengthMetres).succeeded(),"reject Kelvin as a length unit");
    check(!units.normalize(std::numeric_limits<double>::infinity(),"m",Kind::LengthMetres).succeeded(),"reject nonfinite measurement");
    check(!units.normalize(1,"Pa",Kind::Dimensionless).succeeded(),"reject pressure as dimensionless");
    PreviewTimelineController clock;
    PreviewTimelineState t;
    check(!clock.advance(t,.1f,true,false).request_grammar_sample,"paused clock does not sample");
    t.playing=true;
    auto a=clock.advance(t,.025f,true,false);
    check(a.request_grammar_sample && near(a.grammar_sample_time,.025) && near(a.physics_delta_seconds,0),"grammar timeline emits sample only");
    auto b=clock.advance(t,.025f,false,false);
    check(!b.request_grammar_sample && near(b.physics_delta_seconds,.025),"physics timeline emits delta only");
    auto busy=clock.advance(t,.025f,false,true);
    check(near(busy.physics_delta_seconds,0),"physics delta held during generation");
    t.time_scale=2;
    check(near(clock.stepPhysicsTime(t),2.0/60.0),"manual physics step respects time scale");
    clock.reset(t);
    check(!t.playing && t.grammar_target_time==0 && t.simulation_accumulator==0,"reset restores timeline origin");
    std::cout << "Passed " << passed << " checks.\n";
}
