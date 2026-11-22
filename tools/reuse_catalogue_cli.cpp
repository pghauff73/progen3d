#include "AppPaths.h"
#include "grammar/reuse/GrammarReuseCatalogue.h"
#include <iostream>
int main(int argc, char** argv) {
    progen3d_initialize_paths(argv[0]);
    if (auto status = GrammarReuseCatalogue::runCommand(argc, argv)) return *status;
    std::cerr << "Usage: p3d-reuse --list-reuse [text] | --export-reuse REF | --expand-reuse FILE\n";
    return 2;
}
