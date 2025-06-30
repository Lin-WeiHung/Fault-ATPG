#include "AddressAllocator.hpp"
#include "DetectionReport.hpp"
#include "Fault.hpp"
#include "FaultConfig.hpp"
#include "March.hpp"
#include "MemoryState.hpp"
#include "ResultCollector.hpp"
#include "SequenceExecutor.hpp"

class IFaultSimulator {
public:
    virtual ~IFaultSimulator() = default;
    // Execute a March test with the given fault configuration and memory state.
    virtual void init(const std::vector<FaultConfig>& faultConfigs,
                      const std::vector<MarchElement>& marchTest,
                      int rows, int cols, int seed) = 0;
    virtual void run() = 0;
private:
    // Helper method to allocate addresses for faults based on their configuration.
    virtual std::pair<int, int> allocateAddress(const FaultConfig& config) = 0;
    // Helper method to create a memory state based on the given dimensions.
    virtual std::shared_ptr<MemoryState> createMemoryState(int rows, int cols) = 0;
};