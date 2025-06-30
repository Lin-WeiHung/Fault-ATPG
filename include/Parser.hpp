#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "FaultConfig.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;


// Parses JSON input for faults and test patterns, and writes output.
class Parser {
public:
    // Parse fault configurations from a JSON file.
    std::vector<FaultConfig> parseFaults(const std::string& filename) const;

    // Parse a test pattern (sequence of SingleOp) from a JSON file.
    std::vector<MarchElement> parseMarchTest(const std::string& filename) const;

    // Write detection results (syndrome, coverage) to an output file.
private:
    // 共用小工具（與 JSON 庫無關）
    int                toInt(const std::string& raw) const;                 // "-" → -1
    SingleOp           toSingleOp(char opKind, char value) const;           // R0 / W1 …
    std::vector<SingleOp> explodeOpToken(const std::string& token) const;   // R0W1 → {R0,W1}
};

#endif // PARSER_H