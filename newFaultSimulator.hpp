#ifndef FAULT_SIMULATOR_HPP
#define FAULT_SIMULATOR_HPP

#include <vector>
#include <memory>
#include <deque>
#include <map>
#include <unordered_map>
#include <set>
#include <random>
#include <utility>

// 操作類型
enum class OperationType { READ, WRITE };

// 用於 marchOrder 和 opOrder 的組合作為 key
struct MarchOpIdx {
    int marchOrder;
    int opOrder;

    bool operator==(const MarchOpIdx& other) const {
        return marchOrder == other.marchOrder && opOrder == other.opOrder;
    }
    bool operator<(const MarchOpIdx& other) const {
        return std::tie(marchOrder, opOrder) < std::tie(other.marchOrder, other.opOrder);
    }
};

// 單一 March 操作
struct SingleOp {
    OperationType type;
    int value;
    MarchOpIdx idx; // 用於標記在 March test 中的順序
};

// March 操作序列：包含多個 SingleOp
struct MarchElement {
    std::vector<SingleOp> ops; // 單一 March 操作序列
    bool is_asend; // 是否為上升序列
};

// 記錄一次操作前後的狀態
struct OperationRecord {
    int beforeValue;
    SingleOp op;
    int afterValue;
};

// Fault subcase definition
enum class FaultType { SINGLE, COUPLING };

// 用於 FaultType 和 subcaseIdx 的組合作為 key
struct FaultID {
    std::string faultName; // Fault 名稱
    int subcaseIdx;

    bool operator==(const FaultID& other) const {
        return faultName == other.faultName && subcaseIdx == other.subcaseIdx;
    }
};

// 提供給 unordered_map 使用的 hash 函數
namespace std {
    template <>
    struct hash<FaultID> {
        std::size_t operator()(const FaultID& k) const {
            return (std::hash<std::string>()(k.faultName) << 32) ^ std::hash<int>()(k.subcaseIdx);
        }
    };
}

// Fault 子案例：包含 aggressor/victim、trigger sequence、最終值設定
struct FaultFeature {
    FaultType type; // Fault 類型
    int A;   // 0: aggressor < victim; 1: aggressor
    int AI;   // aggressor 需要的值
    int VI;   // victim 需要的值
    std::vector<SingleOp> seqA;
    std::vector<SingleOp> seqV;
    int finalF;   // 觸發後 cell value
    int finalR;   // READ 時回傳值
};

// 扁平化偵測結果，方便 GUI 顯示
struct DetectionRecord {
    FaultID name_and_idx; // FaultIdx 組合
    std::map<MarchOpIdx, int> syndrome; // MarchOpIdx 與偵測結果的對應
    std::set<int> vicAddrs;
};

// Matcher 介面：只定義 match/reset
class ITriggerMatcher {
public:
    virtual ~ITriggerMatcher() = default;
    virtual bool match(const OperationRecord& rec) = 0;
    virtual void reset() = 0;
};

// 僅根據 beforeValue 比對的 Matcher
class ValueMatcher : public ITriggerMatcher {
    int triggerValue_{-1}; // -1 means no value set
public:
    ValueMatcher(int tv);
    bool match(const OperationRecord& rec) override;
    void reset() override;
};

// 序列比對 Matcher（依序比對一段 SingleOp 序列，且第一次 beforeValue 要等於 triggerValue）
class SequenceMatcher : public ITriggerMatcher {
    std::deque<OperationRecord> buffer_;
    std::vector<SingleOp> seq_;
    int triggerValue_{-1}; // -1 means no sequence set
public:
    SequenceMatcher(const std::vector<SingleOp>& seq, int tv);
    bool match(const OperationRecord& rec) override;
    void reset() override;
};

// 單一記憶體 cell，負責 applyOp 與觸發 matcher
class Cell {
    int value_{0};
    int finalCellValue_{-1}, finalReadValue_{-1};
    std::unique_ptr<ITriggerMatcher> matcher_;
public:
    Cell();

    // 安裝 fault：注入 Matcher、fault 後與 READ 回傳值
    void installFault(std::unique_ptr<ITriggerMatcher> m,
                      int finalCellValue, int finalReadValue);

    // 執行單一操作，回傳 READ 的結果或 -1
    int applyOp(const SingleOp& op);

    // 清除 matcher 狀態
    void clearMatcher();
};

// Fault Simulator 主流程
class FaultSimulator {
    int rows_, cols_;
    std::mt19937_64 rng_;
    std::unordered_map<FaultID, DetectionRecord> detectionRecords_; // 偵測結果
    std::map<int, Cell> mem; // 建記憶體 cells
public:
    FaultSimulator(int rows, int cols, uint64_t seed);

    void runSimulation(
        std::unordered_map<FaultID, FaultFeature>& features,
        std::vector<MarchElement>& marchSeq);
private:
    // 模擬單一 subcase
    void simulateSubcase(const FaultFeature& sc, const std::vector<MarchElement>& marchSeq);
    // 選 aggressor/victim 地址
    std::pair<int,int> chooseAggVictim(const FaultFeature& sc);
};

#endif // FAULT_SIMULATOR_HPP
