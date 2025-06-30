#ifndef RESULT_COLLECTOR_H
#define RESULT_COLLECTOR_H

#include <unordered_map>
#include "DetectionReport.hpp"

// Collects fault detection results during simulation.
class IResultCollector {
public:
    virtual void opDetected(const MarchIdx& idx, int addr) = 0;
    virtual ~IResultCollector() = default;
protected:
    DetectionReport results_; // Store detection results
};

class ResultCollector : public IResultCollector {
public:
    void createReport(const std::vector<MarchElement>& marchElements);

    void opDetected(const MarchIdx& idx, int addr) override;
    // Build and return the final detection report.
    DetectionReport getReport() const { return results_; }

private:
    // Map of fault IDs to their detection reports
    DetectionReport results_;
};

#endif // RESULT_COLLECTOR_H