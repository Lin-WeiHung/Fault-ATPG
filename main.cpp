#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "nlohmann/json.hpp"
#include "faultSimulator.hpp"   // existing simulator classes and defs
#include "parser.hpp"

int main(int argc, char** argv) {
    Parser parser(argc, argv);
    // 1. load fault definitions
    const auto &faultList = parser.parseFaultFile();
    // 2. hardcoded March string
    // March test algorithms menu
    const auto &marchSeq = parser.parseMarchString();
    // 3. run simulation
    FaultSimulator sim(marchSeq, faultList);
    const auto& syndromes = sim.runAll();
    // 4. output syndromes
    parser.parserOutSyndrome(syndromes, faultList);
    return 0;
}
