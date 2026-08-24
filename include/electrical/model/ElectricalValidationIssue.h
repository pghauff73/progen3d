#pragma once

#include <string>
#include <utility>

enum class ElectricalValidationCode
{
	MissingCircuit,
	InvalidControlTarget,
	ControlCycle,
	InvalidDimmer,
	EmptyCircuit,
	MissingControl,
	InvalidFixture
};

class ElectricalValidationIssue
{
public:
	ElectricalValidationIssue(ElectricalValidationCode code, std::string message)
		: code_(code), message_(std::move(message)) {}
	ElectricalValidationCode code() const { return code_; }
	const std::string &message() const { return message_; }

private:
	ElectricalValidationCode code_ = ElectricalValidationCode::MissingCircuit;
	std::string message_;
};
