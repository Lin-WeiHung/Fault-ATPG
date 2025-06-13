#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "nlohmann/json.hpp"
#include "faultSimulator.hpp"   // existing simulator classes and defs

using json = nlohmann::json;

class Parser {
public:
    Parser(int argc, char** argv);
    // Parse fault.json into vector<FaultPrimitive>
    std::vector<FaultPrimitive> parseFaultFile();
    
    // Parse a March algorithm string into a sequence of elements
    std::vector<MarchElement> parseMarchString();

    // Parse a syndrome map
    void parserOutSyndrome(const Syndromes& All_syndromes, const std::vector<FaultPrimitive>& faultList);
private:
    std::vector<std::string> files;
    std::string marchTestName;     
};