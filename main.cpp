#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono> // 新增
#include "nlohmann/json.hpp"
#include "faultSimulator.hpp"
#include "parser.hpp"
#include "MarchGenerator.hpp"

int main(int argc, char** argv) {
    Parser parser(argc, argv);
    // 1. load fault definitions
    const auto &faultList = parser.parseFaultFile();
    // 2. hardcoded March string
    // March test algorithms menu
    const auto &marchSeq = parser.parseMarchString();
    // 3. run simulation
    FaultSimulator sim(faultList);

    MarchGenerator gen(sim);
    int length;
    std::cout << "Enter length: ";
    std::cin >> length;

    // 開始計時
    auto start = std::chrono::high_resolution_clock::now();
    gen.generate(length);
    // 結束計時
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Generate time: " << duration.count() << " ms" << std::endl;

    // sim.setMarchSequence(marchSeq);
    // sim.runAll();
    // // 4. output syndromes
    parser.parserOutSyndrome(sim.getAllSyndromes(), faultList);
    return 0;
}
