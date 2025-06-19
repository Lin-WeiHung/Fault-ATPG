#include "newFaultSimulator.hpp"
#include <algorithm>
#include <stdexcept>

// ---------- ValueMatcher 實作 ----------
ValueMatcher::ValueMatcher(int tv)
  : triggerValue_(tv) {}

bool ValueMatcher::match(const OperationRecord& rec) {
    // 只要 beforeValue 等於 trigger 就觸發
    return rec.beforeValue == triggerValue_;
}

void ValueMatcher::reset() {
    // 無狀態需清除
}

// ---------- SequenceMatcher 實作 ----------
SequenceMatcher::SequenceMatcher(const std::vector<SingleOp>& seq, int tv)
  : seq_(seq), triggerValue_(tv) {}

bool SequenceMatcher::match(const OperationRecord& rec) {
    // 先把這次 op 放入 buffer
    buffer_.push_back(rec);
    if (buffer_.size() > seq_.size())
        buffer_.pop_front();

    // 若 buffer 滿長度且 beforeValue 符合 trigger，就檢查 sequence
    if (buffer_.size() == seq_.size() && buffer_.front().beforeValue == triggerValue_) {
        return std::equal(buffer_.begin(), buffer_.end(), seq_.begin(),
            [](const OperationRecord& a, const SingleOp& b){
                return a.op.type==b.type && a.op.value==b.value;
            });
    }
    return false;
}

void SequenceMatcher::reset() {
    buffer_.clear();
}

// ---------- Cell 實作 ----------
Cell::Cell() = default;

void Cell::installFault(std::unique_ptr<ITriggerMatcher> m,
                        int faultValue, int finalReadValue)
{
    matcher_ = std::move(m);
    finalCellValue_ = faultValue;
    finalReadValue_ = finalReadValue;
}

int Cell::applyOp(const SingleOp& op) {
    int before = value_;
    if (op.type == OperationType::WRITE)
        value_ = op.value;

    OperationRecord rec{before, op, value_};

    // 若 match 成功，就把 cell 改為 faultValue
    if (matcher_ && matcher_->match(rec)) {
        value_ = finalCellValue_;
        if (op.type == OperationType::READ && finalReadValue_ != -1) {
            // 如果是 READ 操作，且有設定 finalReadValue，則回傳這個值
            return finalReadValue_;
        }
    }
    return (op.type == OperationType::READ) ? value_ : -1;
}

void Cell::clearMatcher() {
    if (matcher_) matcher_->reset();
}

// ---------- FaultSimulator 實作 ----------
FaultSimulator::FaultSimulator(int rows, int cols, uint64_t seed)
  : rows_(rows), cols_(cols), rng_(seed) {
    mem.clear(); // 清空記憶體
}

std::pair<int,int> FaultSimulator::chooseAggVictim(const FaultFeature& sc) {
    // 隨機選擇 victim/agg obey sc.A
    if (sc.type == FaultType::SINGLE) {
        // 單一 fault: 沒有 aggressor
        std::uniform_int_distribution<int> dist(0, rows_ * cols_ - 1);
        int vic = dist(rng_);
        return {-1, vic}; // -1 表示沒有 aggressor
    }
    if (sc.A == 0) {
        // aggressor < victim
        // 隨機選擇一個合法的 victim
        std::uniform_int_distribution<int> dist(1, rows_ * cols_ - 1);
        int vic = dist(rng_);
        if (vic / cols_ == 0) {
            // 如果是第一行，則不能選擇上面
            return {vic - 1, vic}; // 左邊
        } else if (vic % cols_ == 0) {
            // 如果是第一列，則不能選擇左邊
            return {vic - cols_, vic}; // 上面
        } else {
            // 隨機選擇上面或左邊
            std::uniform_int_distribution<int> choice(0, 1);
            if (choice(rng_) == 0) {
                return {vic - cols_, vic}; // 上面
            } else {
                return {vic - 1, vic}; // 左邊
            }
        }
    }
    if (sc.A == 1) {
        // aggressor > victim
        // 隨機選擇一個合法的 victim
        std::uniform_int_distribution<int> dist(0, rows_ * cols_ - 2);
        int vic = dist(rng_);
        if (vic / cols_ == cols_ - 1) {
            // 如果是最後一行，則不能選擇下面
            return {vic + 1, vic}; // 右邊
        } else if (vic % cols_ == cols_ - 1) {
            // 如果是最後一列，則不能選擇右邊
            return {vic + cols_, vic}; // 下面
        } else {
            // 隨機選擇下面或右邊
            std::uniform_int_distribution<int> choice(0, 1);
            if (choice(rng_) == 0) {
                return {vic + cols_, vic}; // 下面
            } else {
                return {vic + 1, vic}; // 右邊
            }
        }
    }
    return {-1, -1}; // 不應該到這裡
}

void FaultSimulator::simulateSubcase(const FaultFeature& sc, const std::vector<MarchElement>& marchSeq) {
    const int initValue = -1; // 初始值設定為 -1
    // 選 aggressor/victim
    auto [aggrAddr, vicAddr] = chooseAggVictim(sc);

    mem.clear(); // 清空記憶體

    if (aggrAddr != -1) mem[aggrAddr] = Cell();
    if (vicAddr != -1) mem[vicAddr] = Cell();

    if (mem.empty()) {
        // 如果沒有 aggressor/victim，則不需要模擬
        throw std::invalid_argument("No aggressor or victim selected for simulation.");
        return;
    }

    if (marchSeq.empty()) {
        // 如果沒有 March sequence，則不需要模擬
        throw std::invalid_argument("March sequence is empty.");
        return;
    }

    if (sc.seqV.empty()) {
        mem[vicAddr].installFault(
            std::make_unique<ValueMatcher>(sc.VI), sc.finalF, sc.finalR);
    } else {
        // 安裝 aggressor 的 matcher（使用 seqA）
        mem[vicAddr].installFault(
            std::make_unique<SequenceMatcher>(sc.seqV,  sc.VI), sc.finalF, sc.finalR);
    }

    if (aggrAddr != -1 && !sc.seqA.empty()) {
        // 安裝 aggressor 的 matcher（使用 seqA）
        mem[aggrAddr].installFault(
            std::make_unique<SequenceMatcher>(sc.seqA, sc.AI), initValue, initValue);
    } else if (aggrAddr != -1) {
        // 如果沒有 seqA，則使用 ValueMatcher
        mem[aggrAddr].installFault(
            std::make_unique<ValueMatcher>(sc.AI), initValue, initValue);
    }

    // 執行測試序列並收集偵測結果
    for (const auto& elem : marchSeq) {
        if (elem.is_asend) {
            for (auto it = mem.begin(); it != mem.end(); ++it) {
                // 依照 address 由小到大正序執行
                for (const auto& op : elem.ops) {
                    it->second.applyOp(op);
                }
            }
        } else  {
            for (auto it = mem.rbegin(); it != mem.rend(); ++it) {
                // 依照 address 由大到小倒序執行
                for (const auto& op : elem.ops) {
                    it->second.applyOp(op);
                }
            }
        }
    }
}

// #include <iostream>
// int main() {
//     //測試一下 SequenceMatcher::match
//     SequenceMatcher seqMatcher({{OperationType::WRITE, 1}, {OperationType::READ, 1}}, 0);
//     OperationRecord rec1{0, {OperationType::WRITE, 1}, 1};
//     OperationRecord rec2{1, {OperationType::READ, 1}, 1};

//     bool matched1 = seqMatcher.match(rec1); // 應該不會觸發
//     std::cout <<  "Matched 1: " << matched1 << std::endl;
//     bool matched2 = seqMatcher.match(rec2); // 應該會觸發
//     std::cout <<  "Matched 2: " << matched2 << std::endl;

//     seqMatcher.reset(); // 清除狀態

//     return 0;
// }