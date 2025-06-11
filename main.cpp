#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "nlohmann/json.hpp"
#include "faultSimulator.hpp"   // existing simulator classes and defs

using json = nlohmann::json;

// Parse fault.json into vector<FaultPrimitive>
std::vector<FaultPrimitive> parseFaultFile(const std::string& path) {
    std::ifstream in(path);
    if(!in) throw std::runtime_error("Cannot open fault file: " + path);
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
std::vector<MarchElement> parseMarchString(const std::string& march) {
    std::vector<MarchElement> seq;
    std::stringstream ss(march);
    std::string elem;
    while(std::getline(ss, elem, ';')) {
        MarchElement me;
        me.addr_order = (elem[0] == 'd') ? 1 : 0;
        // strip leading char and parentheses
        size_t p = elem.find('(');
        if(p == std::string::npos) continue;
        std::string ops = elem.substr(p+1, elem.find(')')-p-1);
        std::stringstream oss(ops);
        std::string op;
        while(std::getline(oss, op, ',')) {
            SingleOp so;
            if(op[0] == 'r') so.type = OpType::READ;
            else               so.type = OpType::WRITE;
            so.value = op[1] - '0';
            me.ops.push_back(so);
        }
        seq.push_back(me);
    }
    return seq;
}

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " fault.json\n";
        return 1;
    }
    // 1. load fault definitions
    auto faultList = parseFaultFile(argv[1]);
    // 2. hardcoded March C- string
    /*  {"b(w0);a(r0,w1);d(r1,w0,r0)", "MATS++"},
        {"b(w0);a(r0,w1);d(r1,w0);b(r0)", "March X"},
        {"b(w0);a(r0,w1,r1);d(r1,w0,r0);b(r0)", "March Y"},
        {"b(w0);a(r0,w1);a(r1,w0);d(r0,w1);d(r1,w0);b(r0)", "March C-"},
        {"b(w0);a(r0,w1);a(r1,w0,r0);d(r0,w1);d(r1,w0);b(r0)", "March C- modify"},
        {"b(w0);a(r0,w1,w1,r1,w1,w1,r1,w0,w0,r0,w0,w0,r0,w0,w1,w0,w1);a(r1,w0,w0,r0,w0,w0,r0,w1,w1,r1,w1,w1,r1,w1,w0,w1,w0);d(r0,w1,r1,w1,r1,r1,r1,w0,r0,w0,r0,r0,r0,w0,w1,w0,w1);d(r1,w0,r0,w0,r0,r0,r0,w1,r1,w1,r1,r1,r1,w1,w0,w1,w0);b(r0)", "March MD2"},
        {"b(w0);a(r0,w1,r1,w1,r1,r1);a(r1,w0,r0,w0,r0,r0);d(r0,w1,r1,w1,r1,r1);d(r1,w0,r0,w0,r0,r0);b(r0)","March MD9A"},
        {"b(w0);a(r0,w0,w1,w0,w1,r1,w1,w0,w1,w0,r0,w1,w1,w1);a(r1,w1,w0,w1,w0,r0,w0,w1,w0,w1,r1,w0,w0,w0);d(r0,w1,r1,w1,r1,r1,r1,w0,r0,w0,r0,r0,r0,w1,w1,w1);d(r1,w0,r0,w0,r0,r0,r0,w1,r1,w1,r1,r1,r1,w0,w0,w0,w0);b(r0)", "March WY1"},
        {"b(w0);a(r0,w1,ci0,r1);a(r1,w0,ci0,r0);d(r0,w1);d(r1,w0);b(r0)", "March-DC"},
        {"b(w0);a(r0,ci0,r0,co,w1,ci0,r1,co);a(r1,ci1,r1,co,w0,ci1,r0,co);d(r0,ci0,r0,co,w1,ci0,r1,co);d(r1,ci1,r1,co,w0,ci1,r0,co);b(r0)", "March-CM24"},
        {"b(w0);a(r0,r0,w0,r0,w1);a(r1,r1,w1,r1,w0);d(r0,r0,w0,r0,w1);d(r1,r1,w1,r1,w0);b(r0)", "March SS"}
    */
    const std::string marchCminus = "a(w0);a(r0,r0,w0,r0,w1);a(r1,w1,r1,w0);d(r0,w0,r0,w1);d(r1,w1,r1,w0,w0,r0);d(r0,w0,r0);d(r0,w1,r1);d(r1,w1);a(r1,w0,r0);a(r0,w1,r1);a(r1,w0);d(r0,w0);a(r0,w1);d(r1,w1,r1);d(r1,w0);d(r0)";
    auto marchSeq = parseMarchString(marchCminus);
    // 3. run simulation
    FaultSimulator sim(marchSeq, faultList);
    sim.runAll();
    return 0;
}
