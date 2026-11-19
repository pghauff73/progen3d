#pragma once

#include <optional>
#include <vector>

namespace fff {

// Distances in mm, forces in N, pressures/moduli in MPa, time in s.
struct BeltDrive {
    double massKg, dragN, pretensionN, spanNPerMm, toothNPerMm, mountNPerMm;
    double shaftRadiusMm, shaftLengthMm, shearMPa, backlashMm, jumpN, pitchMm, pulleyTeeth;
    double creepFraction = 0, retardationS = 1;
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
// Series compliance of a confined strand. Moduli are effective axial compression moduli.
struct StockMaterial {
    double densityKgM3, instantaneousMPa, relaxedMPa, retardationS, guidedLengthMm;
};
struct Point { double x, y, z; };
struct Move { double seconds; Point target; double extrusionMultiplier; };
struct PrinterProgram {
    BeltDrive xDrive{}, yDrive{};
    SpurGearPair gears{};
    GuideShaftPair guides{};
    ThermalStack thermal{};
    Hotend hotend{};
    std::optional<StockMaterial> stock;
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
    double creepMm;
};
class BeltTransmission {
public:
    BeltTransmission(BeltDrive model, double home) : model_(model), contact_(home) {}
    AxisResponse transmit(double command, double velocityMmS, double accelerationMmS2, double seconds = 0);
private:
    BeltDrive model_;
    BacklashContact contact_;
    double creepMm_ = 0;
    int signedJumps_ = 0, events_ = 0, latchedDirection_ = 0;
};

struct MeltIncrement { double feedMm3, outMm3, slipMm3; };
class MeltChamber {
public:
    explicit MeltChamber(Hotend model, std::optional<StockMaterial> stock = {});
    double lineStoredMm3() const { return immediateCompliance_*pressure_ + delayedVolume_; }
    double delayedStoredMm3() const { return delayedVolume_; }
    MeltIncrement advance(double attemptedMm3, double seconds);
    double pressureMPa() const { return pressure_; }
private:
    Hotend model_;
    double pressure_ = 0;
    double immediateCompliance_ = 0, delayedCompliance_ = 0, retardationS_ = 1, delayedVolume_ = 0;
};

struct PrinterSample {
    Point command{}, nozzle{};
    AxisResponse x{}, y{};
    double lift = 0, sag = 0, expansion = 0;
    double commandFeed = 0, hobFeed = 0, acceptedFeed = 0, slip = 0;
    double pressure = 0, flow = 0, deposited = 0, ooze = 0, stored = 0;
    double lineCompression = 0, delayedCompression = 0, meltStored = 0, inletFeed = 0, feedForce = 0;
};
}
