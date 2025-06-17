// MarchGenerator.hpp
#ifndef MARCH_GENERATOR_HPP
#define MARCH_GENERATOR_HPP

#include "faultSimulator.hpp"

// Generator for March sequences that achieve 100% detection rate
class MarchGenerator {
public:
    // Constructor takes all fault primitives to test against
    MarchGenerator(FaultSimulator& sim) : simulator(sim) {};

    // Generate a sequence of given length that yields 100% detection
    // Returns empty vector if none found
    std::vector<MarchElement> generate(int length);

    void printMarchTest() const;
private:
    std::vector<SingleOp> current;
    std::vector<MarchElement> marchTest;
    FaultSimulator& simulator; // Reference to the fault simulator

    // Depth-first search to build candidates
    bool dfs(int pos, int length);

    // Check transition constraint between prev and next ops
    bool isValidTransition(const SingleOp& prev, const SingleOp& next);

    void combineMarchElements();

    // Wrap SingleOp vector into MarchElement sequence and test detection
    bool testSequence();
};

#endif // MARCH_GENERATOR_HPP