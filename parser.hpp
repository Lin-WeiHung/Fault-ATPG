#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "nlohmann/json.hpp"
#include "faultSimulator.hpp"   // existing simulator classes and defs

using json = nlohmann::json;

class Parser {
    std::vector<std::string> files;
    std::string marchTestName;
    std::vector<FaultID> faultIDs;
public:
    Parser(int argc, char** argv);
    // Parse fault.json into 
    std::unordered_map<FaultID, std::unique_ptr<BaseFault>> parseFaultFile();

    // Parse a March algorithm string into a sequence of elements
    std::vector<MarchElement> parseMarchString();

    // Parse a syndrome map
    void parserOutSyndrome(const std::unordered_map<FaultID, std::unique_ptr<BaseFault>>& faults);
private:
    std::unique_ptr<BaseFault> createFaults(const FaultID& faultID, const std::vector<std::string>& fields);
};