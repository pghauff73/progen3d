#pragma once
#include "mechanics/model/PrinterDrive.h"
#include <string>
#include <map>
#include <vector>

namespace fff {
class PrinterResponse {
public:
    explicit PrinterResponse(const PrinterProgram &program, double maxStep = .005);
    double duration() const { return times_.back(); }
    static bool recognizes(const std::string &name);
    double value(const std::string &name, double time) const;
    const PrinterSample &last() const { return samples_.back(); }
private:
    std::map<std::string,double> constants_;
    std::vector<double> times_;
    std::vector<PrinterSample> samples_;
};

// Reads one declarative program while preserving the line numbers of native geometry rules.
class PrinterProgramReader {
public:
    static bool isDirective(const std::string &line);
    static PrinterProgram read(std::vector<std::string> &geometryLines, int &diagnosticLine);
};
}
