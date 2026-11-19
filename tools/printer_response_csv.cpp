// Sample the same FFF program and response channels used by the native editor.
#include "mechanics/service/PrinterResponse.h"
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc,char **argv) {
    if (argc!=2) { std::cerr<<"Usage: printer-response-csv file.p3d\n"; return 2; }
    try {
        std::ifstream source(argv[1]);
        if (!source) throw std::runtime_error("cannot open grammar");
        std::vector<std::string> lines;
        for (std::string line;std::getline(source,line);) lines.push_back(line);
        int where=0;
        fff::PrinterResponse response(fff::PrinterProgramReader::read(lines,where));
        const std::vector<std::string> channels={"CommandX","CommandY","CommandZ","X","Y","Z",
            "XTight","XSlack","YTight","YSlack","XStretch","XTooth","XShear","XTwist","Sag","Expansion",
            "Pressure","Flow","CommandFeed","HobFeed","Feed","Slip","Deposit","Ooze","Stored","XJumps","YJumps","GearGap"};
        std::cout<<"seconds";
        for (const auto &c:channels) std::cout<<','<<c;
        std::cout<<'\n'<<std::setprecision(12);
        for (int i=0;;++i) {
            const double t=std::min(response.duration(),i/16.);
            std::cout<<t;
            for (const auto &c:channels) std::cout<<','<<response.value("fff"+c,t);
            std::cout<<'\n';
            if (t==response.duration()) break;
        }
    } catch (const std::exception &error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}
