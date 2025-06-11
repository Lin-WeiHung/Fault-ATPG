#ifndef FAULT_SIMULATOR_SYSTEM_HPP
#define FAULT_SIMULATOR_SYSTEM_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <array>
#include <queue>
#include <map>

using namespace std;

constexpr int MAX_ADDR = 16;

enum class OpType { READ, WRITE };
struct SingleOp {
    OpType type;
    int    value;   // 0 or 1
};

// record each operation on a cell
typedef struct OperationRecord {
    int beforeValue;    // cell value before op
    SingleOp op;        // the operation
    int afterValue;     // cell value after op
} OperationRecord;

// Fault subcase definition
enum class FaultType { SINGLE, COUPLING };
struct FaultSubcase {
    FaultType type;
    int        A;       // 0: aggr<vic; 1: aggr>vic; -1 none
    int        AI;      // required aggressor value
    int        VI;      // required victim value
    vector<SingleOp> seqA;
    vector<SingleOp> seqV;
    SingleOp         opD;
    int              finalF;
    int              finalR;
};

// March test element
typedef vector<SingleOp> MarchElement;

// Cell simulation with queue-based FSM
class Cell {
public:
    Cell();
    void installFault(const FaultSubcase* f);
    void clearQueue();
    void applyOp(const SingleOp& op);
    int  read();
    void write(int v);
    void setLockResetValue(int resetVal);
    void setMaxQueueSize(size_t sz);
private:
    int value;
    const FaultSubcase* fault;
    queue<OperationRecord> history;
    size_t maxQueue;
    bool triggered;
    
    void recordOperation(const SingleOp& op, int before);
    bool checkTrigger();
    void triggerFault();
    bool historyMatches(const vector<SingleOp>& seq) const;
};

// Memory holds 16 cells and routes operations
class Memory {
public:
    Memory(int init);
    void installFault(const FaultSubcase* f, int vicAddr, int aggrAddr);
    void clearElementQueues();
    void writeCell(int addr, int v);
    int  readCell(int addr);
private:
    array<Cell, MAX_ADDR> cells;
    int victimAddr;
    int aggressorAddr;
};

// Fault primitive and simulation
struct Syndrome {
    bool detected; 
    vector<int> addresses;
};
struct FaultPrimitive {
    string name;
    vector<FaultSubcase> subs;
};

class FaultSimulator {
public:
    FaultSimulator(const vector<MarchElement>& m,
                   const vector<FaultPrimitive>& f);
    void runAll();
private:
    vector<MarchElement> marchSeq;
    vector<FaultPrimitive> faults;
    mt19937 rng;

    pair<int,int> chooseAggVictim(const FaultSubcase& sc);
    void simulateSubcase(const FaultSubcase& sc, int init);
};

#endif // FAULT_SIMULATOR_SYSTEM_HPP
