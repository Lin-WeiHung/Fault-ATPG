#include "parser.hpp"

Parser::Parser(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <fault model .json> <march test .json>\n";
        throw std::runtime_error("Invalid arguments");
    }
    for (int i = 1; i < argc; ++i) {
        files.push_back(argv[i]);
    }
}
// Parse fault.json into unordered_map<FaultID, std::unique_ptr<BaseFault>>
std::unordered_map<FaultID, std::unique_ptr<BaseFault>> Parser::parseFaultFile() {
    std::ifstream in(files[0]);
    if(!in) throw std::runtime_error("Cannot open fault file: " + files[0]);
    json jf;
    in >> jf;
    std::unordered_map<FaultID, std::unique_ptr<BaseFault>> faultMap;
    for(auto& jfp : jf) {
        std::string name = jfp.at("name").get<std::string>();
        int subcaseIdx = 0;
        for(auto& cond : jfp.at("conditions")) {
            std::string condStr = cond.get<std::string>();
            std::vector<std::string> fields;
            std::stringstream ss(condStr);
            std::string field;
            while (std::getline(ss, field, ',')) {
                field.erase(0, field.find_first_not_of(" \t\n\r{}"));
                field.erase(field.find_last_not_of(" \t\n\r{}") + 1);
                fields.push_back(field);
            }
            FaultID fid{name, subcaseIdx++};
            faultMap[fid] = createFaults(fid, fields);
            faultIDs.push_back(fid);
        }
    }
    return faultMap;
}

// Parse a March algorithm string into a sequence of elements
std::vector<MarchElement> Parser::parseMarchString() {
    std::ifstream in(files[1]);
    if (!in) throw std::runtime_error("Cannot open march test file: " + files[1]);
    json jf;
    in >> jf;

    std::vector<std::string> marchNames;
    for (const auto& j : jf) {
        marchNames.push_back(j.at("name").get<std::string>());
    }

    std::cout << "Available March patterns:\n";
    for (size_t i = 0; i < marchNames.size(); ++i) {
        std::cout << i + 1 << ". " << marchNames[i] << "\n";
    }
    std::cout << "Select a March pattern by number: ";
    size_t choice = 0;
    std::cin >> choice;
    if (choice < 1 || choice > marchNames.size()) {
        throw std::runtime_error("Invalid selection");
    }
    marchTestName = marchNames[choice - 1];

    std::string pattern;
    bool found = false;
    for (const auto& j : jf) {
        if (j.at("name") == marchTestName) {
            pattern = j.at("pattern").get<std::string>();
            found = true;
            break;
        }
    }
    if (!found) throw std::runtime_error("March pattern not found: " + marchTestName);

    std::vector<MarchElement> seq;
    std::stringstream ss(pattern);
    std::string elem;
    int operationCount = 1;
    // Split by semicolon
    while(std::getline(ss, elem, ';')) {
        MarchElement me;
        me.is_asend = (elem[0] == 'd') ? 0 : 1;
        size_t p = elem.find('(');
        if(p == std::string::npos) continue;
        std::string ops = elem.substr(p+1, elem.find(')')-p-1);
        std::stringstream oss(ops);
        std::string op;
        while(std::getline(oss, op, ',')) {
            SingleOp so;
            if(op[0] == 'r') so.type = OperationType::READ;
            else             so.type = OperationType::WRITE;
            so.value = op[1] - '0';
            me.ops.push_back(so);
        }
        seq.push_back(me);
    }
    return seq;
}

// Parse out a syndrome map
void Parser::parserOutSyndrome(const std::unordered_map<FaultID, std::unique_ptr<BaseFault>>& faults) {
    std::ofstream ofs(marchTestName + "_syndromes.txt");
    if (!ofs) {
        std::cerr << "Cannot open output file for writing.\n";
        return;
    }
    for (const auto& faultID : faultIDs) {
        const auto& fault = faults.at(faultID);
        // Output fault information
        ofs << faultID.faultName_ << " Subcase " << faultID.subcaseIdx_;
        ofs << fault->getTriggerInfo();
        // Output syndrome
        if (!fault->isDetected()) {
            ofs << "No detection\n";
            continue;
        }
        ofs << " Syndrome: ";
        for (const auto& it : fault->getDetectionRecord().syndrome) {
            // Collect syndrome bits in order of marchOrder and opOrder
            std::vector<std::pair<int, int>> orders;
            for (const auto& it : fault->getDetectionRecord().syndrome) {
                orders.emplace_back(it.first.marchOrder, it.first.opOrder);
            }
            // Sort by marchOrder then opOrder
            std::sort(orders.begin(), orders.end());
            std::string bits;
            for (const auto& ord : orders) {
                auto it = fault->getDetectionRecord().syndrome.find({ord.first, ord.second});
                bits += (it != fault->getDetectionRecord().syndrome.end() && it->second == 1) ? '1' : '0';
            }
            // Convert bits string to hex
            if (!bits.empty()) {
                unsigned long long value = std::stoull(bits, nullptr, 2);
                ofs << "0x" << std::hex << value << std::dec;
            }
        }
        for (const auto& it : fault->getDetectionRecord().syndrome) {
            if (it.second == 1) {
                ofs << "M" << it.first.marchOrder << "(" << it.first.opOrder << ") ";
            }
        }
        ofs << "\n";
    }
}

std::unique_ptr<BaseFault> Parser::createFaults(const FaultID& faultID, const std::vector<std::string>& fields) {
    if (fields.size() == 5) {
        // Single cell fault
        auto fault = std::make_unique<OneCellFault>(faultID);
        fault->reset();
        std::vector<SingleOp> seqV;
        if (fields[1] != "-") {
            std::stringstream ss(fields[1]);
            std::string op;
            while (std::getline(ss, op, ',')) {
                if (op[0] == 'R') {
                    seqV.push_back({OperationType::READ, std::stoi(op.substr(1))});
                } else {
                    seqV.push_back({OperationType::WRITE, std::stoi(op.substr(1))});
                }
            }
        }
        fault->setTrigger(std::stoi(fields[0]), seqV);
        fault->setFaultValue(std::stoi(fields[3]));
        if (fields[4] != "-") {
            fault->setFinalReadValue(std::stoi(fields[4]));
        }
        return fault;
    }

    if (fields.size() == 8) {
        // Coupling fault
        auto fault = std::make_unique<TwoCellFault>(faultID);
        fault->reset();
        std::vector<SingleOp> seqA, seqV;
        if (fields[3] != "-") {
            std::stringstream ss(fields[3]);
            std::string op;
            while (std::getline(ss, op, ',')) {
                if (op[0] == 'R') {
                    seqA.push_back({OperationType::READ, std::stoi(op.substr(1))});
                } else {
                    seqA.push_back({OperationType::WRITE, std::stoi(op.substr(1))});
                }
            }
            fault->setFaultType(TwoCellFaultType::Saa, std::stoi(fields[0]) ? true : false); // Aggressor < Victim
            fault->setTrigger(std::stoi(fields[1]), std::stoi(fields[2]), seqA);
        }
        if (fields[4] != "-") {
            std::stringstream ss(fields[4]);
            std::string op;
            while (std::getline(ss, op, ',')) {
                if (op[0] == 'R') {
                    seqV.push_back({OperationType::READ, std::stoi(op.substr(1))});
                } else {
                    seqV.push_back({OperationType::WRITE, std::stoi(op.substr(1))});
                }
            }
            fault->setFaultType(TwoCellFaultType::Svv, std::stoi(fields[0]) ? true : false); // Aggressor > Victim
            fault->setTrigger(std::stoi(fields[1]), std::stoi(fields[2]), seqV);
        }
        fault->setFaultValue(std::stoi(fields[6]));
        if (fields[7] != "-") {
            fault->setFinalReadValue(std::stoi(fields[7]));
        }
        return fault;
    }

    throw std::runtime_error("Unsupported fields size for fault creation.");
}
