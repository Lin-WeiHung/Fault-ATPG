#include "../include/SequenceExecutor.hpp"

void SequenceExecutor::execute( const std::vector<MarchElement>& marchTest, IFault& fault) {
    if (memSize_ <= 0 || marchTest.empty()) {
        // No memory to simulate or no operations to execute
        return;
    }
    for (const auto& elem : marchTest) {
        auto iterate = [&](auto begin, auto end, auto step){
            for (auto addr = begin; addr != end; addr += step)
                processElementAtAddr(elem, fault, addr);
        };
        if (elem.addrOrder_ == Direction::ASC || elem.addrOrder_ == Direction::BOTH)
            iterate(size_t{0}, memSize_, size_t{1});
        if (elem.addrOrder_ == Direction::DESC)
            iterate(memSize_-1, size_t(-1), size_t(-1));
    }
}

void SequenceExecutor::processElementAtAddr(const MarchElement& elem, IFault& fault, int mem_idx) {
    for (const auto& op : elem.ops_) {
        if (op.op_.type_ == OpType::R) {
            // Read operation
            int value = fault.readProcess(mem_idx, op.op_);
            if (value != op.op_.value_) {
                collector_.opDetected(op.idx_, mem_idx);
            }
        } else if (op.op_.type_ == OpType::W) {
            // Write operation
            fault.writeProcess(mem_idx, op.op_);
        }
    }
}