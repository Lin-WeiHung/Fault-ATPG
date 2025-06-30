# include "../include/ResultCollector.hpp"

void ResultCollector::createReport(const std::vector<MarchElement>& marchElements) {
    // Initialize the results_ report based on the marchElements
    for (const auto& elem : marchElements) {
        for (const auto& OpC : elem.ops_) {
            if (OpC.op_.type_ == OpType::R) { results_.detected_[OpC.idx_] = false; }
        }
    }
}
    
void ResultCollector::opDetected(const MarchIdx& idx, int addr) {
    if (results_.detected_.find(idx) == results_.detected_.end()) {
        return; // If idx is not found, do nothing
    }
    // Mark the operation as detected in the results report
    results_.detected_[idx] = true;
    results_.isDetected_ = true; // At least one detection occurred
    results_.detectedVicAddrs_.insert(addr); // Add the detected victim address
}