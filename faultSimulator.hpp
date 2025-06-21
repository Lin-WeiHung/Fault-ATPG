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

    bool operator==(const OperationRecord& other) const {
        return beforeValue == other.beforeValue && op.type == other.op.type && op.value == other.op.value;
    }
};

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

// 扁平化偵測結果，方便 GUI 顯示
struct DetectionRecord {
    std::map<MarchOpIdx, int> syndrome; // MarchOpIdx 與偵測結果的對應
    std::set<int> vicAddrs;
};

constexpr int UNPROCESS = -9999; // 用於表示錯誤的值
// --------------------------------------------------
// BaseFault：所有 fault type 的抽象基底
// --------------------------------------------------
class BaseFault {
protected:
    const FaultID& faultID_; // 用於識別的 FaultID
    std::map<int, int> mem_; // 模擬的記憶體 cells
    int faultValue_, finalReadValue_;  // fault 後的 cell value 與 READ 回傳值
    bool isTriggered_;  // 是否已觸發過
    bool isDetected_; // 是否已偵測到 fault
    DetectionRecord detectionRecord_; // 偵測結果記錄
public:
    BaseFault(const FaultID& faultID)
        : faultID_(faultID), isTriggered_(false), isDetected_(false), faultValue_(UNPROCESS), finalReadValue_(UNPROCESS) {
    }

    virtual ~BaseFault() = default;

    // 重置各種狀態，供下一次模擬或同一 subcase 重啟
    virtual void reset() = 0;

    // 是否已觸發過 fault
    virtual bool isTriggered() const { return isTriggered_; }

    // 是否已偵測到 fault
    virtual bool isDetected() const { return isDetected_; }

    // Read 事件回調；若偵測到 fault，回傳 != -1，並推一筆 DetectionRecord
    virtual int onRead(int addr, const MarchOpIdx& opIdx, const SingleOp& op) = 0;

    // Write 事件回調；可在此觸發 fault injection 等
    virtual void onWrite(int addr, const SingleOp& op) = 0;

    // TriggerMatcher
    virtual bool triggerMatcher() = 0;

    // 取得當前map
    virtual const std::map<int, int>& getMemory() const { return mem_; }

    // 設置 fault 值與最終讀取值
    virtual void setFaultValue(int faultValue) { faultValue_ = faultValue; }
    virtual void setFinalReadValue(int finalReadValue) { finalReadValue_ = finalReadValue; }

    // 取得 fault info
    virtual std::string getTriggerInfo() const = 0;
    virtual DetectionRecord getDetectionRecord() const { return detectionRecord_; }
};

// --------------------------------------------------
// OneCellFault：單一 cell 的 fault 模擬
// 流程：
// 1. 建立 OneCellFault 實例
// 2. reset() 重置狀態
// 3. setTrigger(const int VI, const std::vector<SingleOp>& seq) 設定觸發條件序列
// 4. setFaultValue(int faultValue) 設定 fault 值
// 5. setFinalReadValue(int finalReadValue) 設定最終讀取值 (可選)
// 6. 定義完成後
//    - 在模擬過程
//      - setVictimState(int vic, int init) 設定 victim cell 的地址並初始化
//      - onRead(int addr, const MarchOpIdx& opIdx, const SingleOp& op) 讀取操作
//      - onWrite(int addr, const SingleOp& op) 寫入操作
// 7. 在模擬過程中，若觸發條件匹配，則會注入 fault 值到 victim cell
// 8. 若偵測到 fault，則會在 onRead 中回傳非 -1 的值，並推入 DetectionRecord
// 9. 可透過 isTriggered() 和 isDetected() 檢查是否已觸發或偵測到 fault
// 10. 可透過 getMemory() 取得當前記憶體狀態
// 11. 可透過 reset() 重置狀態以便進行下一次模擬或同一 subcase 重啟
// --------------------------------------------------
class OneCellFault : public BaseFault {
    int vic_; // victim cell 的地址
    std::deque<OperationRecord> trigger_; // 觸發條件序列
    std::deque<OperationRecord> history_; // 操作歷史記錄
public:
    OneCellFault(const FaultID& faultID)
      : BaseFault(faultID) {
        reset(); // 初始化時重置狀態
    }

    void reset() override;

    int onRead(int addr, const MarchOpIdx& opIdx, const SingleOp& op) override;

    void onWrite(int addr, const SingleOp& op) override;

    bool triggerMatcher() override;

    void setTrigger(const int VI, const std::vector<SingleOp>& seq); 

    void setVictimState(int vic, int init) { vic_ = vic; mem_[vic] = init; } // 設定 victim cell 的地址並初始化

    std::string getTriggerInfo() const override;
};

// TwoCellFaultType: Saa (aggressor-aggressor) or Svv (victim-victim)
enum class TwoCellFaultType { Saa, Svv };

// --------------------------------------------------
// TwoCellFault：雙 cell 的 fault 模擬
// 流程：
// 1. 建立 TwoCellFault 實例
// 2. Reset() 重置狀態
// 5. setFaultType(TwoCellFaultType type) 設定 Fault 類型 (Saa 或 Svv)
// 6. setTrigger(const int AI, const int VI, const std::vector<SingleOp>& seq) 設定觸發條件序列
// 7. setFaultValue(int faultValue) 設定 fault 值
// 8. setFinalReadValue(int finalReadValue) 設定最終讀取值 (可選)
// 9. 定義完成後
//    - 在模擬過程
//      - setAggressorState(int aggr, int init, bool is_A_less_than_V) 設定 aggressor cell 的地址並初始化
//      - setVictimState(int vic, int init) 設定 victim cell 的地址並初始化
//      - onRead(int addr, const MarchOpIdx& opIdx, const SingleOp& op) 讀取操作
//      - onWrite(int addr, const SingleOp& op) 寫入操作
// 10. 在模擬過程中，若觸發條件匹配，則會注入 fault 值到 victim cell
// 11. 若偵測到 fault，則會在 onRead 中回傳非 -1 的值，並推入 DetectionRecord
// 12. 可透過 isTriggered() 和 isDetected() 檢查是否已觸發或偵測到 fault
// 13. 可透過 getMemory() 取得當前記憶體狀態
// 14. 可透過 reset() 重置狀態以便進行下一次模擬或同一 subcase 重啟
// --------------------------------------------------
class TwoCellFault : public BaseFault {
    bool is_A_less_than_V_; // 是否 aggressor < victim
    int aggr_, vic_; // aggressor 和 victim cell 的地址
    TwoCellFaultType type_; // Fault 類型：Saa 或 Svv
    int coupleTriggerValue_; // couple trigger value
    std::deque<OperationRecord> trigger_; // 觸發條件序列
    std::deque<OperationRecord> history_; // 操作歷史記錄
public:
    TwoCellFault(const FaultID& faultID)
      : BaseFault(faultID) {
        reset(); // 初始化時重置狀態
    }

    void reset() override;

    int onRead(int addr, const MarchOpIdx& opIdx, const SingleOp& op) override;

    void onWrite(int addr, const SingleOp& op) override;

    bool triggerMatcher() override;

    void setTrigger(const int AI, const int VI, const std::vector<SingleOp>& seq);

    void setFaultType(TwoCellFaultType type, bool is_A_less_than_V) { type_ = type; is_A_less_than_V_ = is_A_less_than_V; } // 設定 Fault 類型
    void setAggressorState(int aggr, int init) { aggr_ = aggr; mem_[aggr] = init; } // 設定 aggressor cell 的地址並初始化
    void setVictimState(int vic, int init) { vic_ = vic; mem_[vic] = init; } // 設定 victim cell 的地址並初始化

    std::string getTriggerInfo() const override;
};

// --------------------------------------------------
// Fault Simulator : 
// --------------------------------------------------
class FaultSimulator {
    int rows_, cols_;
    std::mt19937_64 rng_;
public:
    FaultSimulator(int rows, int cols, uint64_t seed)
        : rows_(rows), cols_(cols), rng_(seed) {
    }

    void runAll(std::vector<MarchElement>& marchSeq,
                std::unordered_map<FaultID, std::unique_ptr<BaseFault>>& faults);

private:
    // 模擬單一 subcase
    void simulateSubcase(std::vector<MarchElement>& marchSeq,
                         std::unique_ptr<BaseFault>& faults);
    // 選 aggressor/victim 地址
    void chooseAggVictim(const std::unique_ptr<BaseFault>& fault, int init);
};

#endif // FAULT_SIMULATOR_HPP
