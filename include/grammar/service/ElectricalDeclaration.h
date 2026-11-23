#pragma once

#include "electrical/model/ElectricalMachineModel.h"
#include "grammar.h"
#include "grammar/service/SpatialDeclarationParserTypes.h"

// Numeric fields are evaluated in the lexical frame of the expanded rule.
// References are linked in scene preflight so source order is immaterial.
class ElectricalDeclaration : public GrammarActionToken {
public:
    enum class Kind { Wire, Coil, Magnet, Motor, Shaft };
    static std::optional<Kind> recognize(const std::string &name, const std::vector<std::string> &tokens, std::size_t index);
    ElectricalDeclaration(const std::string &name, Kind kind) : GrammarActionToken(name), kind(kind) {}
    GrammarActionToken *clone() const override { return new ElectricalDeclaration(*this); }
    std::string isRule() override { return {}; }
    std::string print() override;
    void performAction(SceneGenerationContext *) override {} // Committed as a complete model during preflight.
    bool parse(const std::vector<std::string> &tokens, std::size_t *index,
               const SpatialDeclarationParserTypes::ExpressionArgumentParser &parseExpression, std::string &diagnostic);
    std::string addTo(ElectricalMachineModel &scene) const;

    std::string attachShaft(ElectricalMachineModel &scene, std::size_t bodyIndex,
                            bool fixed, bool hasInertia) const;
    const std::string &objectId() const { return id_; }
    Kind kind;
    std::map<std::string, std::string> expressions;
    std::map<std::string, double> values;
private:
    std::string id_;
    std::map<std::string, std::string> identifiers_;
    std::string addWire(ElectricalMachineModel &scene) const;
    std::string addCoil(ElectricalMachineModel &scene) const;
    std::string addMagnet(ElectricalMachineModel &scene) const;
    std::string addMotor(ElectricalMachineModel &scene) const;
};
