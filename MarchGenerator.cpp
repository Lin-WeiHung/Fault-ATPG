// MarchGenerator.cpp
#include "MarchGenerator.hpp"

MarchGenerator::MarchGenerator(const std::vector<FaultPrimitive>& faultPrims)
    : faults(faultPrims) {}

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
        if (testSequence(current)) return true;
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

bool MarchGenerator::testSequence(const std::vector<SingleOp>& seq) {
    // wrap as MarchElement list
    std::vector<MarchElement> marchSeq;
    marchSeq.reserve(seq.size());
    for (auto& op : seq) {
        MarchElement m;
        m.addr_order = true;
        m.ops = { op };
        marchSeq.push_back(m);
    }
    // run simulator
    FaultSimulator sim(marchSeq, faults);
    auto& synd = sim.runAll();
    // count total subcases and detected
    int total = 0;
    int detected = 0;
    for (auto& fp : faults) {
        for (auto& subMap : synd[fp.name]) {
            for (auto& kv : subMap) {
                total++;
                if (kv.second.detected) detected++;
            }
        }
    }
    return (detected == total);
}