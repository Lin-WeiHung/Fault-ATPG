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
    auto faultList = parser.parseFaultFile();
    // 2. hardcoded March string
    // March test algorithms menu
    auto marchSeq = parser.parseMarchString();
    // 3. run simulation
    FaultSimulator sim(4, 4, 123456789); // 假設 4x4 的記憶體陣列，隨機種子為 123456789

    // MarchGenerator gen(sim);
    // int length;
    // std::cout << "Enter length: ";
    // std::cin >> length;

    // // 開始計時
    // auto start = std::chrono::high_resolution_clock::now();
    // gen.generate(length);
    // // 結束計時
    // auto end = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // std::cout << "Generate time: " << duration.count() << " ms" << std::endl;

    sim.runAll(marchSeq, faultList);
    // // 4. output syndromes
    parser.parserOutSyndrome(faultList);
    return 0;
}
