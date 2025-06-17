#ifndef FAULT_SIMULATOR_SYSTEM_HPP
#define FAULT_SIMULATOR_SYSTEM_HPP

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <array>
#include <map>
#include <deque>

constexpr int MAX_ADDR = 16;

enum class OpType { READ, WRITE };
typedef struct singleOp {
    OpType type;
    int    value;   // 0 or 1
    int    order;   // 在整個March test 中的順序
} SingleOp;

// record each operation on a cell
typedef struct operationRecord {
    int beforeValue;    // cell value before op
    SingleOp op;        // the operation
    int afterValue;     // cell value after op
} OperationRecord;

// Fault subcase definition
enum class FaultType { SINGLE, COUPLING };
typedef struct faultSubcase {
    FaultType type;
    int        A;       // 0: aggr<vic; 1: aggr>vic; -1 none
    int        AI;      // required aggressor value
    int        VI;      // required victim value
    std::vector<SingleOp> seqA;
    std::vector<SingleOp> seqV;
    SingleOp         opD;
    int              finalF;
    int              finalR;
} FaultSubcase;

// March test element
typedef struct marchElement {
    bool addr_order; // true: b or a, false: d
    std::vector<SingleOp> ops; // operations on this element
} MarchElement;

// Cell simulation with queue-based FSM
class Cell {
public:
    Cell();
    void init(int init);
    void installFault(const std::vector<SingleOp>& seq, int TriggerI, int finalF, int finalR);
    void installFault(int TriggerI, int finalF, int finalR);
    void clearQueue();
    bool checkTrigger();
    int applyOp(const SingleOp& op);
private:
    int cell_value;
    int TriggerValue;
    std::vector<SingleOp> faultSeq;
    int faultValue;
    int finalReadValue;
    std::deque<OperationRecord> history;
    size_t maxQueue;
    void recordOperation(const SingleOp& op, int before);
    void triggerFault();
    bool historyMatches() const;
};

// Memory holds 16 cells and routes operations
class Memory {
public:
    Memory(int init);
    void installFault(const FaultSubcase* f, int vicAddr, int aggrAddr);
    void clearElementQueues();
    void writeCell(int addr, int v);
    int  readCell(int addr, int v);
private:
    std::array<Cell, MAX_ADDR> cells;
};

// Fault primitive and simulation
typedef struct opSynd {
    opSynd() : detected(false) {}
    bool detected; 
    std::vector<int> addresses;
} OpSynd;

typedef struct faultPrimitive {
    std::string name;
    std::vector<FaultSubcase> subs;
} FaultPrimitive;

typedef std::map<int, OpSynd> SubcaseSynd;
typedef std::map<std::string, std::vector<SubcaseSynd>> Syndromes;

class FaultSimulator {
public:
    FaultSimulator(const std::vector<FaultPrimitive>& f);
    void setFaults(const std::vector<FaultPrimitive>& f) { faults = f; }
    void setMarchSequence(const std::vector<MarchElement>& m) { marchSeq = m; }
    void runAll();
    const Syndromes& getAllSyndromes() const { return All_syndromes; }
private:
    std::vector<MarchElement> marchSeq;
    std::vector<FaultPrimitive> faults;
    std::mt19937 rng;
    Syndromes All_syndromes;
    std::vector<int> iterateAddresses(bool addr_order);
    std::pair<int,int> chooseAggVictim(const FaultSubcase& sc);
    void simulateSubcase(std::vector<SubcaseSynd>& subcaseSynd_vec, const FaultSubcase& sc, int init);
};

#endif // FAULT_SIMULATOR_SYSTEM_HPP
