#pragma once

#include <vector>

namespace fff {

// Distances in mm, forces in N, pressures/moduli in MPa, time in s.
struct BeltDrive {
    double massKg, dragN, pretensionN, spanNPerMm, toothNPerMm, mountNPerMm;
    double shaftRadiusMm, shaftLengthMm, shearMPa, backlashMm, jumpN, pitchMm, pulleyTeeth;
};
struct SpurGearPair {
    double moduleMm, pinionTeeth, wheelTeeth, pressureAngleDeg;
    double pinionThinningMm, wheelThinningMm, centreOffsetMm, pitchErrorMm, hobRadiusMm;
    double backlashMm() const;
    double wheelRadiusMm() const { return moduleMm * wheelTeeth / 2; }
};
struct GuideShaftPair {
    double spanMm, diameterMm, modulusMPa, carriageKg;
    double sagMm(double x) const;
};
struct ThermalStack { double lengthMm, expansionPerK, hotMinusReferenceK; };
struct Hotend { double filamentDiameterMm, complianceMm3PerMPa, resistanceMPaSPerMm3, nipN, friction; };
struct Point { double x, y, z; };
struct Move { double seconds; Point target; double extrusionMultiplier; };
struct PrinterProgram {
    BeltDrive xDrive{}, yDrive{};
    SpurGearPair gears{};
    GuideShaftPair guides{};
    ThermalStack thermal{};
    Hotend hotend{};
    double nutBacklashMm = 0, widthMm = 0, heightMm = 0;
    Point home{};
    std::vector<Move> moves;
};

class BacklashContact {
public:
    explicit BacklashContact(double initial = 0) : output_(initial) {}
    double transmit(double input, double gap);
private:
    double output_;
};

struct AxisResponse {
    double position, strandPosition, tightN, slackN, stretchMm, toothMm, shearMm, torsionMm;
    int jumps;
};
class BeltTransmission {
public:
    BeltTransmission(BeltDrive model, double home) : model_(model), contact_(home) {}
    AxisResponse transmit(double command, double velocityMmS, double accelerationMmS2);
private:
    BeltDrive model_;
    BacklashContact contact_;
    int signedJumps_ = 0, events_ = 0, latchedDirection_ = 0;
};

struct MeltIncrement { double feedMm3, outMm3, slipMm3; };
class MeltChamber {
public:
    explicit MeltChamber(Hotend model) : model_(model) {}
    MeltIncrement advance(double attemptedMm3, double seconds);
    double pressureMPa() const { return pressure_; }
private:
    Hotend model_;
    double pressure_ = 0;
};

struct PrinterSample {
    Point command{}, nozzle{};
    AxisResponse x{}, y{};
    double lift = 0, sag = 0, expansion = 0;
    double commandFeed = 0, hobFeed = 0, acceptedFeed = 0, slip = 0;
    double pressure = 0, flow = 0, deposited = 0, ooze = 0, stored = 0;
};
}
