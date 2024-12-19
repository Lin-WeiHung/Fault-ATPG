#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <unordered_map>
#include <vector>
#include <sstream>

struct Fault {
    std::string Sa;
    std::string Sv;
};

// Function declarations
class FaultParser {
public:
    FaultParser();
    bool parseFP(char* argv[]);
    void printFaults();
private:
    std::vector<Fault> transformFP(const std::string& fp);
    std::pair<std::string, std::string> get_faultname_FP(const std::string& line);
    std::unordered_map<std::string, std::vector<Fault>> faults;
};

// Constructor
FaultParser::FaultParser() {}

// Function to parse the input file
bool FaultParser::parseFP(char* argv[]) {
    std::ifstream infile(argv[1]); // Read faults.txt 
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file!" << std::endl;
        return false;
    }
    std::string line;
    std::string current_fault_name;

    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        std::string faultname, fp;
        // Extract the fault name and FP
        std::tie(faultname, fp) = get_faultname_FP(line);
        if (faultname.empty()) {
            std::cout << "Invalid format: " << line << std::endl;
            continue;
        }
        std::vector<Fault> FPs = transformFP(fp);
        faults[faultname] = FPs;
    }

    return true;
}

// Function to extract the fault name and FP from a line
std::pair<std::string, std::string> FaultParser::get_faultname_FP(const std::string& line) {
    // Regex pattern to extract the fault name and FP
    std::regex pattern(R"((.*?)\s*<\s*(.*))");
    std::smatch match;

    if (std::regex_search(line, match, pattern)) {
        std::string faultname = match[1].str();
        std::string fp = "<" + match[2].str(); // Add the < back to the FP
        return std::make_pair(faultname, fp);
    } else {
        // 如果沒有找到 <，返回空字串
        return std::make_pair("", "");
    }
}

// Function to parse and transform a single FP
std::vector<Fault> FaultParser::transformFP(const std::string& fp) {
    // Regex patterns for <Sv/F/R> and <Sa; Sv/F/R>
    std::regex pattern(R"((.*?>)\s*(.*))");
    std::regex pattern_single_cell(R"(<([^/]+)/[^>]*>)");
    std::regex pattern_multi_cell(R"(<([^;]+); ([^/]+?)/[^>]*>)");

    std::smatch match;

    std::vector<Fault> FPs;

    std::string remaining_fp = fp;
    std::string cur_fp;
    while (std::regex_search(remaining_fp, match, pattern)) {
        cur_fp = match[1].str();
        remaining_fp = match[2].str();
        if (std::regex_match(cur_fp, match, pattern_multi_cell)) {
            std::string Sa = match[1].str();
            std::string Sv = match[2].str();
            FPs.push_back({Sa, Sv});
        } else if (std::regex_match(cur_fp, match, pattern_single_cell)) {
            std::string Sv = match[1].str();
            FPs.push_back({"-", Sv});
        } else {
            std::cout << "Invalid format: " << cur_fp << std::endl;
        }
    }
    return FPs;

}

// Function to print the faults
void FaultParser::printFaults() {
    for (const auto& [faultname, FPs] : faults) {
        std::cout << "Fault: " << faultname << ", #FP: " << FPs.size() << std::endl;
        for (const auto& fp : FPs) {
            std::cout << "Sa: " << fp.Sa << ", Sv: " << fp.Sv << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    FaultParser parser;
    if (!parser.parseFP(argv)) {
        return 1;
    }

    parser.printFaults();

    return 0;
}