// MarchGenerator.cpp
#include "MarchGenerator.hpp"

std::vector<MarchElement> MarchGenerator::generate(int length) {
    current.clear();
    if (length <= 0) return {};
    // start DFS
    if (dfs(0, length)) {
        // wrap into MarchElement sequence
        std::vector<MarchElement> result;
        for (auto& op : current) {
            MarchElement m;
            m.addr_order = true;  // default order
            m.ops = { op };
            result.push_back(m);
        }
        return result;
    }
    return {};
}

bool MarchGenerator::dfs(int pos, int length) {
    // If built full sequence, test
    if (pos == length) {
        combineMarchElements();
        if (testSequence()) {
            printMarchTest();  //----------------------print for debugging
            return true;
        }
        return false;
    }
    // iterate all possible ops
    static const std::vector<SingleOp> allOps = {
        {OpType::WRITE, 0, 0},
        {OpType::WRITE, 1, 0},
        {OpType::READ,  0, 0},
        {OpType::READ,  1, 0}
    };
    for (auto& op : allOps) {
        if (pos > 0 && !isValidTransition(current[pos-1], op))
            continue;
        current.push_back(op);
        if (dfs(pos+1, length)) return true;
        current.pop_back();
    }
    return false;
}

bool MarchGenerator::isValidTransition(const SingleOp& prev, const SingleOp& next) {
    // w0 -> r1 forbidden
    if (prev.type == OpType::WRITE && prev.value == 0 && next.type == OpType::READ && next.value == 1)
        return false;
    // r0 -> r1 forbidden
    if (prev.type == OpType::READ && prev.value == 0 && next.type == OpType::READ && next.value == 1)
        return false;
    // w1 -> r0 forbidden
    if (prev.type == OpType::WRITE && prev.value == 1 && next.type == OpType::READ && next.value == 0)
        return false;
    // r1 -> r0 forbidden
    if (prev.type == OpType::READ && prev.value == 1 && next.type == OpType::READ && next.value == 0)
        return false;
    return true;
}

bool MarchGenerator::testSequence() {
    // run simulator
    simulator.setMarchSequence(marchTest);
    simulator.runAll();
    // get all syndromes
    const Syndromes& synd = simulator.getAllSyndromes();
    if (synd.empty()) {
        std::cout << "No syndromes detected, sequence is invalid." << std::endl;
        return false; // no syndromes means no detection
    }
    // count total subcases and detected
    int total = 0;
    int detected = 0;
    for (const auto& [faultname, subcaseSynd_vec] : synd) {
        total += subcaseSynd_vec.size();
        std::vector<int> detectedVec;
        for (const auto& subcaseSynd : subcaseSynd_vec) {
            for (const auto& [order, synd] : subcaseSynd) {
                if (synd.detected) {
                    detected++;
                    break; // only need one detected per subcase
                }
            }
        }
        bool detect = std::any_of(detectedVec.begin(), detectedVec.end(), [](int b){ return b == 1; });
    }
    return (detected == total);
}

void MarchGenerator::combineMarchElements() {
    marchTest.clear();
    int x = 0; // default value for WRITE
    int notx = 1; // default value for READ
    const bool ADDR_ANY = 0; // address order: any
    const bool ADDR_UP = 0; // address order: up
    const bool ADDR_DOWN = 1; // address order: down
    // construct S and ~S sequences
    std::vector<SingleOp> notS;
    for (auto& op : current) notS.push_back({op.type, (op.value ^ 1), 0});
    int Dk = current.empty() ? x : current.back().value;
    int notDk = Dk ^ 1;
    auto makeElem = [&](bool order, std::initializer_list<SingleOp> ops) {
        MarchElement m; m.addr_order = order; m.ops = ops; return m;
    };
    // TAT elements: adapt presence rules as needed
    marchTest.push_back(makeElem(ADDR_ANY, { {OpType::WRITE, notDk, 0} }));
    marchTest.push_back(makeElem(ADDR_UP,    std::initializer_list<SingleOp>{{OpType::READ, notDk,0},{OpType::WRITE,x,0}}));
    // add S
    for (auto& op: current) marchTest.back().ops.push_back(op);
    marchTest.push_back(makeElem(ADDR_UP,    std::initializer_list<SingleOp>{{OpType::READ, Dk,0},{OpType::WRITE, notx ,0}}));
    for (auto& op: notS) marchTest.back().ops.push_back(op);
    marchTest.push_back(makeElem(ADDR_DOWN,  std::initializer_list<SingleOp>{{OpType::READ, notDk,0},{OpType::WRITE,x,0}}));
    for (auto& op: current) marchTest.back().ops.push_back(op);
    marchTest.push_back(makeElem(ADDR_DOWN,  std::initializer_list<SingleOp>{{OpType::READ, Dk,0},{OpType::WRITE, notx,0}}));
    for (auto& op: notS) marchTest.back().ops.push_back(op);
    marchTest.push_back(makeElem(ADDR_ANY,   { {OpType::READ, notDk,0} }));
}

void MarchGenerator::printMarchTest() const {
    std::cout << "Generated March Test Sequence:-----------------------------------\n";
    for (const auto& elem : marchTest) {
        std::cout << "Address order: " << (elem.addr_order ? "UP" : "DOWN") << "\n";
        for (const auto& op : elem.ops) {
            std::cout << (op.type == OpType::READ ? "R" : "W") << op.value << " ";
        }
        std::cout << "\n";
    }
}