#include "grammar/service/ElectricalDeclaration.h"
#include "SpatialDeclarationFieldReader.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace {
using Kind = ElectricalDeclaration::Kind;
struct DeclarationFields {
    std::set<std::string> names, numbers, required;
};
const DeclarationFields &fieldsFor(Kind kind) {
    static const DeclarationFields wire{
        {"material", "insulation"}, {"diameter", "thickness", "temperature"},
        {"material", "insulation", "diameter", "thickness"}};
    static const DeclarationFields coil{
        {"wire", "core"}, {"turns", "turnLength", "coreLength", "area", "gap", "current", "insulationVoltage"},
        {"wire", "core", "turns", "turnLength", "coreLength", "area", "gap", "current"}};
    static const DeclarationFields magnet{
        {"material", "returnCore"}, {"length", "returnLength", "area", "gap", "polarity"},
        {"material", "returnCore", "length", "returnLength", "area", "gap"}};
    static const DeclarationFields motor{
        {"type", "connection", "winding", "magnet", "barMaterial"},
        {"polePairs", "rpm", "windingFactor", "bars", "barLength", "barArea", "barTemperature",
         "barLeakage", "barEmf", "frequency"},
        {"type", "connection", "winding", "polePairs", "rpm"}};
    static const DeclarationFields shaft{{"role"}, {"axis"}, {"role"}};
    switch (kind) {
    case Kind::Shaft: return shaft;
    case Kind::Wire: return wire;
    case Kind::Coil: return coil;
    case Kind::Magnet: return magnet;
    case Kind::Motor: return motor;
    }
    return motor;
}

double valueOr(const std::map<std::string, double> &values, const std::string &key, double fallback) {
    const auto found = values.find(key);
    return found == values.end() ? fallback : found->second;
}
bool wholePositive(double value) {
    return value >= 1 && value <= std::numeric_limits<unsigned>::max() && std::floor(value) == value;
}
bool validTemperature(double temperature, const ConductorMaterial &material) {
    return temperature > -273.15 && material.resistivityAt(temperature) > 0;
}
}

std::optional<Kind> ElectricalDeclaration::recognize(const std::string &name, const std::vector<std::string> &tokens, std::size_t index) {
    // Conventional rules may already use these names with positional arguments.
    // An electrical declaration starts with an ID followed by a named field.
    if (index + 3 >= tokens.size() || tokens[index] != "(" || tokens[index + 3] != "(") return std::nullopt;
    Kind kind;
    if (name == "MagnetWire") kind = Kind::Wire;
    else if (name == "FieldCoil") kind = Kind::Coil;
    else if (name == "PermanentMagnet") kind = Kind::Magnet;
    else if (name == "ElectricMotor") kind = Kind::Motor;
    else if (name == "MotorShaft") kind = Kind::Shaft;
    else return std::nullopt;
    const auto &field = tokens[index + 2];
    const auto &fields = fieldsFor(kind);
    if (!fields.names.count(field) && !fields.numbers.count(field)) return std::nullopt;
    return kind;
}

std::string ElectricalDeclaration::print() {
    std::string text = token_name + "(" + id_;
    for (const auto &[name, value] : identifiers_) text += " " + name + "(" + value + ")";
    if (kind == Kind::Shaft && expressions.count("axisX")) {
        text += " axis(" + expressions.at("axisX") + " " + expressions.at("axisY") + " " + expressions.at("axisZ") + ")";
    } else {
        for (const auto &[name, expr] : expressions) text += " " + name + "(" + expr + ")";
    }
    return text + ") ";
}

bool ElectricalDeclaration::parse(const std::vector<std::string> &tokens, std::size_t *index,
        const SpatialDeclarationParserTypes::ExpressionArgumentParser &parseExpression, std::string &diagnostic) {
    using Reader = SpatialDeclarationFieldReader;
    if (!Reader::beginDeclaration(tokens, index, token_name, &diagnostic) ||
        !Reader::readIdentifier(tokens, index, token_name, &id_, &diagnostic)) return false;
    const auto &schema = fieldsFor(kind);
    std::set<std::string> seen;
    while (*index < tokens.size() && tokens[*index] != ")") {
        std::string field;
        std::size_t start = 0;
        if (!Reader::beginField(tokens, index, &field, &start, &diagnostic)) return false;
        if (!seen.insert(field).second) {
            diagnostic = token_name + " repeats field '" + field + "'.";
            return false;
        }
        if (schema.names.count(field)) {
            if (!Reader::readIdentifier(tokens, index, field, &identifiers_[field], &diagnostic)) return false;
        } else if (kind == Kind::Shaft && field == "axis") {
            for (const char *component : {"axisX", "axisY", "axisZ"}) {
                GeometryExpression expression("");
                if (!Reader::readExpression(tokens, index, field, parseExpression, &expression, &diagnostic)) return false;
                expressions[component] = expression.sourceText();
            }
        } else if (schema.numbers.count(field)) {
            GeometryExpression expr("");
            if (!Reader::readExpression(tokens, index, field, parseExpression, &expr, &diagnostic)) return false;
            expressions[field] = expr.sourceText();
        } else {
            diagnostic = token_name + " has no field '" + field + "'.";
            return false;
        }
        if (!Reader::endField(tokens, index, field, &diagnostic)) return false;
    }
    for (const auto &field : schema.required) {
        if (!seen.count(field)) {
            diagnostic = token_name + " requires field '" + field + "'.";
            return false;
        }
    }
    return Reader::endField(tokens, index, token_name, &diagnostic);
}

std::string ElectricalDeclaration::addWire(ElectricalMachineModel &scene) const {
    const auto conductor = ElectricalMaterialCatalog::conductor(identifiers_.at("material"));
    const auto insulation = ElectricalMaterialCatalog::insulation(identifiers_.at("insulation"));
    if (!conductor || !insulation) return "unknown conductor or insulation material";
    MagnetWire wire{conductor, insulation, values.at("diameter"), values.at("thickness"), valueOr(values,"temperature",20)};
    if (wire.diameter <= 0 || wire.insulationThickness <= 0) return "diameter and thickness must be positive";
    if (!validTemperature(wire.temperature, *conductor)) return "invalid temperature for conductor resistivity";
    scene.wires.emplace(id_, wire);
    return {};
}
std::string ElectricalDeclaration::addCoil(ElectricalMachineModel &scene) const {
    const auto wireId = identifiers_.at("wire");
    const auto core = ElectricalMaterialCatalog::core(identifiers_.at("core"));
    if (!scene.wires.count(wireId)) return "unknown MagnetWire '" + wireId + "'";
    if (!core) return "unknown core material '" + identifiers_.at("core") + "'";
    if (!wholePositive(values.at("turns"))) return "turns must be a positive integer";
    FieldCoil coil{wireId, core, static_cast<unsigned>(values.at("turns")), values.at("turnLength"),
                   values.at("coreLength"), values.at("area"), values.at("gap"), values.at("current"),
                   valueOr(values,"insulationVoltage",0)};
    if (coil.meanTurnLength <= 0 || coil.coreLength <= 0 || coil.area <= 0 || coil.gap < 0)
        return "turnLength, coreLength and area must be positive and gap nonnegative";
    scene.coils.emplace(id_, coil);
    return {};
}
std::string ElectricalDeclaration::addMagnet(ElectricalMachineModel &scene) const {
    const auto material = ElectricalMaterialCatalog::magnet(identifiers_.at("material"));
    const auto core = ElectricalMaterialCatalog::core(identifiers_.at("returnCore"));
    if (!material || !core) return "unknown permanent magnet or return core material";
    const double polarity = valueOr(values,"polarity",1);
    if (polarity != 1 && polarity != -1) return "polarity must be +1 or -1";
    PermanentMagnet magnet{material, core, values.at("length"), values.at("returnLength"),
                           values.at("area"), values.at("gap"), static_cast<int>(polarity)};
    if (magnet.length <= 0 || magnet.area <= 0 || magnet.returnLength < 0 || magnet.gap < 0)
        return "length and area must be positive and returnLength and gap nonnegative";
    scene.magnets.emplace(id_, magnet);
    return {};
}

std::string ElectricalDeclaration::addMotor(ElectricalMachineModel &scene) const {
    const auto type = findMotorTopology(identifiers_.at("type"));
    if (!type) return "unknown motor topology '" + identifiers_.at("type") + "'";
    const auto &winding = identifiers_.at("winding");
    if (!scene.coils.count(winding)) return "unknown FieldCoil '" + winding + "'";
    if (!wholePositive(values.at("polePairs"))) return "polePairs must be a positive integer";
    const auto &connection = identifiers_.at("connection");
    const bool dc = *type == MotorTopology::BrushedDC;
    if (dc && connection != "Armature") return "BrushedDC requires connection(Armature)";
    if (!dc && connection != "Wye" && connection != "Delta") return "three-phase motors require Wye or Delta";
    if (*type == MotorTopology::SixStepOutrunner && connection != "Wye")
        return "SixStepOutrunner uses two conducting phases in Wye";
    ElectricMotor motor{*type, WindingConnection::Armature, winding, {},
                        static_cast<unsigned>(values.at("polePairs")), values.at("rpm"),
                        valueOr(values,"windingFactor",1), std::nullopt};
    if (!dc) motor.connection = connection == "Wye" ? WindingConnection::Wye : WindingConnection::Delta;
    const bool induction = motor.topology == MotorTopology::InductionCage;
    for (const auto *field : {"bars", "barLength", "barArea", "barLeakage", "barEmf", "frequency"}) {
        if (induction != bool(values.count(field)))
            return std::string(induction ? "InductionCage requires " : "PM motor cannot use ") + field;
    }
    if (induction) {
        if (identifiers_.count("magnet") || values.count("windingFactor")) return "magnet and windingFactor apply only to PM motors";
        if (!identifiers_.count("barMaterial")) return "InductionCage requires barMaterial";
        const auto conductor = ElectricalMaterialCatalog::conductor(identifiers_.at("barMaterial"));
        if (!conductor) return "unknown cage bar conductor";
        if (!wholePositive(values.at("bars"))) return "bars must be a positive integer";
        RotorCage cage{conductor, static_cast<unsigned>(values.at("bars")), values.at("barLength"),
                       values.at("barArea"), valueOr(values,"barTemperature",20), values.at("barLeakage"),
                       values.at("barEmf"), values.at("frequency")};
        if (cage.pathLength <= 0 || cage.barArea <= 0 || cage.leakageInductance < 0 ||
            cage.standstillEmfRms < 0 || cage.supplyFrequency <= 0 || !validTemperature(cage.temperature, *conductor))
            return "invalid bar dimensions, temperature, leakage, RMS EMF or supply frequency";
        motor.cage = cage;
    } else {
        if (identifiers_.count("barMaterial") || values.count("barTemperature")) return "bar fields require InductionCage";
        if (!identifiers_.count("magnet")) return "PM motor requires magnet";
        motor.magnetId = identifiers_.at("magnet");
        if (!scene.magnets.count(motor.magnetId)) return "unknown PermanentMagnet '" + motor.magnetId + "'";
        if (motor.windingFactor <= 0 || motor.windingFactor > 1) return "windingFactor must be in (0, 1]";
    }
    scene.motors.emplace(id_, motor);
    return {};
}

std::string ElectricalDeclaration::addTo(ElectricalMachineModel &scene) const {
    std::string error;
    if (scene.wires.count(id_) || scene.coils.count(id_) || scene.magnets.count(id_) || scene.motors.count(id_))
        error = "duplicate machine object ID";
    else {
        switch (kind) {
        case Kind::Shaft: return "MotorShaft must be attached to a rigid body.";
        case Kind::Wire: error = addWire(scene); break;
        case Kind::Coil: error = addCoil(scene); break;
        case Kind::Magnet: error = addMagnet(scene); break;
        case Kind::Motor: error = addMotor(scene); break;
        }
    }
    return error.empty() ? error : token_name + " '" + id_ + "': " + error + ".";
}

std::string ElectricalDeclaration::attachShaft(ElectricalMachineModel &scene, std::size_t bodyIndex,
                                               bool fixed, bool hasInertia) const {
    if (!scene.motors.count(id_)) return "MotorShaft references unknown motor '" + id_ + "'.";
    const auto &role = identifiers_.at("role");
    if (role != "Rotor" && role != "Stator") return "MotorShaft role must be Rotor or Stator.";
    if (role == "Rotor" && fixed) return "MotorShaft rotor requires a movable I primitive.";
    if (!fixed && !hasInertia) return "A movable MotorShaft body requires BodyInertia.";
    auto &shaft = scene.shafts[id_];
    auto &body = role == "Rotor" ? shaft.rotorBody : shaft.statorBody;
    if (body) return "Motor '" + id_ + "' already has a " + role + " body.";
    if (values.count("axisX")) {
        if (role != "Rotor") return "Specify the MotorShaft axis on the rotor.";
        const double x = values.at("axisX"), y = values.at("axisY"), z = values.at("axisZ");
        const double length = std::hypot(x, y, z);
        if (!(length > 0 && std::isfinite(length))) return "MotorShaft axis must be finite and nonzero.";
        shaft.rotorAxis = glm::vec3(x / length, y / length, z / length);
    }
    body = bodyIndex;
    return {};
}
