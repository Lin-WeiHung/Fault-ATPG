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
// Parse fault.json into vector<FaultPrimitive>
std::vector<FaultPrimitive> Parser::parseFaultFile() {
    std::ifstream in(files[0]);
    if(!in) throw std::runtime_error("Cannot open fault file: " + files[0]);
    json jf;
    in >> jf;
    std::vector<FaultPrimitive> list;
    for(auto& jfp : jf) {
        FaultPrimitive fp;
        fp.name = jfp.at("name").get<std::string>();
        fp.subs.clear();
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
            FaultSubcase sc;
            sc.type   = (fields.size()==8 && fields[0]!="-") ? FaultType::COUPLING : FaultType::SINGLE;
            if (sc.type == FaultType::COUPLING) {
                sc.A      = std::stoi(fields[0]);
                sc.AI     = std::stoi(fields[1]);
                sc.VI     = std::stoi(fields[2]);
                // parse AS into seqA
                if(fields[3] != "-") {
                    std::stringstream ss(fields[3]);
                    std::string op;
                    while(std::getline(ss, op, ',')) {
                        if(op[0]=='R') sc.seqA.push_back({OpType::READ,  op[1]-'0'});
                        else            sc.seqA.push_back({OpType::WRITE, op[1]-'0'});
                    }
                }
                // parse VS into seqV
                if(fields[4] != "-") {
                    std::stringstream ss(fields[4]); std::string op;
                    while(std::getline(ss, op, ',')) {
                        if(op[0]=='R') sc.seqV.push_back({OpType::READ,  op[1]-'0'});
                        else            sc.seqV.push_back({OpType::WRITE, op[1]-'0'});
                    }
                }
                // D into opD
                sc.opD = {OpType::READ, fields[5][1]-'0'};
                sc.finalF = std::stoi(fields[6]);
                sc.finalR = (fields[7] == "-") ? -1 : (fields[7][0]-'0');
            } else if (sc.type == FaultType::SINGLE) {
                sc.A = -1; // not used
                sc.AI = -1; // not used
                sc.VI = std::stoi(fields[0]);
                // parse VS into seqV
                if(fields[1] != "-") {
                    std::stringstream ss(fields[1]); std::string op;
                    while(std::getline(ss, op, ',')) {
                        if(op[0]=='R') sc.seqV.push_back({OpType::READ,  op[1]-'0'});
                        else            sc.seqV.push_back({OpType::WRITE, op[1]-'0'});
                    }
                }
                // D into opD
                sc.opD = {OpType::READ, fields[2][1]-'0'};
                sc.finalF = std::stoi(fields[3]);
                sc.finalR = (fields[4] == "-") ? -1 : (fields[4][0]-'0');
            }
            fp.subs.push_back(std::move(sc));
        }
        list.push_back(std::move(fp));
    }
    return list;
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
        me.addr_order = (elem[0] == 'd') ? 1 : 0;
        size_t p = elem.find('(');
        if(p == std::string::npos) continue;
        std::string ops = elem.substr(p+1, elem.find(')')-p-1);
        std::stringstream oss(ops);
        std::string op;
        while(std::getline(oss, op, ',')) {
            SingleOp so;
            if(op[0] == 'r') so.type = OpType::READ;
            else             so.type = OpType::WRITE;
            so.value = op[1] - '0';
            so.order = operationCount++;
            me.ops.push_back(so);
        }
        seq.push_back(me);
    }
    return seq;
}

// Parse out a syndrome map
void Parser::parserOutSyndrome(const Syndromes& All_syndromes, const std::vector<FaultPrimitive>& faultList) {
    std::ofstream ofs(marchTestName + "_syndromes.txt");
    if (!ofs) {
        std::cerr << "Cannot open output file for writing.\n";
        return;
    }
    for(const auto& fault : faultList) {
        ofs << "Fault: " << fault.name << "\n";
        const auto& it = All_syndromes.find(fault.name);
        if(it == All_syndromes.end()) {
            ofs << "  No syndromes detected for this fault.\n";
            continue;
        }
        const auto& subcaseSynd_vec = it->second;
        for (size_t i = 0; i < subcaseSynd_vec.size(); i += 2) {
            ofs << "  Subcase " << (i / 2) + 1 << ": ";
            // 印出subcase的基本資料
            if (i / 2 < fault.subs.size()) {
                const auto& sub = fault.subs[i / 2];
                if (sub.type == FaultType::COUPLING) {
                    ofs << "A: " << sub.A << ", AI: " << sub.AI << ", VI: " << sub.VI << "\n";
                } else {
                    ofs << "    VI: " << sub.VI << "\n";
                }
            } else {
                ofs << "\n";
            }
            // 印出init 0 和 1 的syndrome
            for (int init = 0; init < 2; ++init) {
                if (i + init >= subcaseSynd_vec.size()) break;
                const auto& subcaseSynd = subcaseSynd_vec[i + init];
                std::vector<int> detectedVec;
                for (const auto& [order, synd] : subcaseSynd) {
                    detectedVec.push_back(synd.detected ? 1 : 0);
                }
                bool detect = std::any_of(detectedVec.begin(), detectedVec.end(), [](int b){ return b == 1; });
                ofs << "    Init " << init << " syndrome: ";
                if (detect) {
                    for (int d : detectedVec) ofs << d;
                    // Output hexadecimal version
                    int value = 0;
                    for (int d : detectedVec) {
                        value = (value << 1) | d;
                    }
                    ofs << " (0x" << std::hex << value << std::dec << ")";
                    ofs << "\n";
                } else {
                    ofs << "undetected\n";
                }
            }
        }
        ofs << "\n";
    }
}