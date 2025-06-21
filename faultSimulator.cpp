#include "faultSimulator.hpp"

// -------------------------------
// OneCellFault class implementation
// -------------------------------
void OneCellFault::reset() {
    vic_ = UNPROCESS; // 默認為未設定
    trigger_ = {};
    history_ = {};
    isTriggered_ = false;
}

int OneCellFault::onRead(int addr, const MarchOpIdx& opIdx, const SingleOp& op) {
    if (addr != vic_) {
        std::__throw_invalid_argument("Address does not match victim cell address.");
    }
    // 記錄操作前的值
    if (history_.size() >= trigger_.size()) {
        history_.pop_front(); // 保持歷史記錄長度
    }
    history_.push_back({mem_[addr], op});

    // 檢查是否觸發 fault
    if (addr == vic_ && triggerMatcher()) {
        isTriggered_ = true;
        mem_[addr] = faultValue_; // 注入 fault 值
        if (finalReadValue_ == UNPROCESS) {
            // 如果未設定 finalReadValue，則錯誤
            std::__throw_invalid_argument("Final read value is not set.");
        }
        if (op.value != finalReadValue_) {
            // 如果 READ 的值不等於預期的 finalReadValue，則記錄偵測結果
            isDetected_ = true;
            detectionRecord_.syndrome[opIdx] = true; // 偵測到 fault
            detectionRecord_.vicAddrs.insert(addr); // 記錄 victim 地址
            return faultValue_; // 回傳 fault 值
        } else {
            // 如果 detectionRecord_.syndrome 沒有 opIdx 這個 key，則新增
            if (detectionRecord_.syndrome.find(opIdx) == detectionRecord_.syndrome.end()) {
                detectionRecord_.syndrome[opIdx] = false; // 未偵測到
            }
            return mem_[addr]; // 回傳預期值
        }
    }

    if (mem_[addr] != op.value) {
        // 如果讀取的值與預期不符，則記錄偵測結果
        isDetected_ = true;
        detectionRecord_.syndrome[opIdx] = true; // 偵測到 fault
        detectionRecord_.vicAddrs.insert(addr); // 記錄 victim 地址
    } else {
        // 如果 detectionRecord_.syndrome 沒有 opIdx 這個 key，則新增
        if (detectionRecord_.syndrome.find(opIdx) == detectionRecord_.syndrome.end()) {
            detectionRecord_.syndrome[opIdx] = false; // 未偵測到
        }
    }

    return mem_[addr]; // 回傳預期值
}

void OneCellFault::onWrite(int addr, const SingleOp& op) {
    if (addr != vic_) {
        std::__throw_invalid_argument("Address does not match victim cell address.");
    }
    if (history_.size() >= trigger_.size()) {
        history_.pop_front(); // 保持歷史記錄長度
    }
    history_.push_back({mem_[addr], op});
    // 記錄寫入操作
    mem_[addr] = op.value;
    if (addr == vic_ && triggerMatcher()) {
        mem_[addr] = faultValue_; // 注入 fault 值
    }
}

bool OneCellFault::triggerMatcher() {
    if (trigger_.size() != history_.size()) {
        return false; // 如果歷史記錄長度不等於觸發條件序列長度，則不匹配
    }
    return std::equal(trigger_.begin(), trigger_.end(), history_.begin(),
        [](const OperationRecord& a, const OperationRecord& b) {
            return a == b; // 比較操作前的值和操作類型
        });
}

void OneCellFault::setTrigger(const int VI, const std::vector<SingleOp>& seq) {
    trigger_.clear();
    for (int i = 0; i < seq.size(); ++i) {
        if (i == 0) {
            trigger_.push_back({VI, seq[i]});
            continue;
        }
        trigger_.push_back({seq[i - 1].value, seq[i]});
    }
}

std::string OneCellFault::getTriggerInfo() const {
    std::string seq = std::to_string(trigger_.front().beforeValue);
    for (const auto& rec : trigger_) {
        seq += rec.op.type == OperationType::READ ? "R" : "W";
        seq += std::to_string(rec.op.value);
    }
    std::string finalF = std::to_string(faultValue_);
    std::string finalR = (finalReadValue_ == UNPROCESS ? "-" : std::to_string(finalReadValue_));
    return "< " + seq + " / " + finalF + " / " + finalR + " >";
}

// -------------------------------
// TwoCellFault class implementation
// -------------------------------

void TwoCellFault::reset() {
    aggr_ = UNPROCESS;
    vic_ = UNPROCESS;
    type_ = TwoCellFaultType::Saa; // 默認為 Saa
    coupleTriggerValue_ = UNPROCESS; // 未設定
    trigger_ = {};
    history_ = {};
    isTriggered_ = false;
}

int TwoCellFault::onRead(int addr, const MarchOpIdx& opIdx, const SingleOp& op) {
    if (addr != vic_ && addr != aggr_) {
        std::__throw_invalid_argument("Address does not match victim or aggressor cell address.");
    }

    if (type_ == TwoCellFaultType::Saa && addr == aggr_) {
        // 如果是 Saa 類型，則只記錄 aggressor 的操作
        if (history_.size() >= trigger_.size()) {
            history_.pop_front(); // 保持歷史記錄長度
        }
        history_.push_back({mem_[addr], op});
    } else if (type_ == TwoCellFaultType::Svv && addr == vic_) {
        // 如果是 Svv 類型，則只記錄 victim 的操作
        if (history_.size() >= trigger_.size()) {
            history_.pop_front(); // 保持歷史記錄長度
        }
        history_.push_back({mem_[addr], op});
    }

    // 檢查是否觸發 fault
    if (((type_ == TwoCellFaultType::Saa && addr == aggr_) || (type_ == TwoCellFaultType::Svv && addr == vic_))
        && triggerMatcher()) {
        isTriggered_ = true;
        mem_[addr] = faultValue_; // 注入 fault 值
        if (type_ == TwoCellFaultType::Svv && finalReadValue_ == UNPROCESS) {
            // 如果未設定 finalReadValue，則錯誤
            std::__throw_invalid_argument("Final read value is not set.");
        }
        if (op.value != finalReadValue_) {
            // 如果 READ 的值不等於預期的 finalReadValue，則記錄偵測結果
            isDetected_ = true;
            detectionRecord_.syndrome[opIdx] = true; // 偵測到 fault
            detectionRecord_.vicAddrs.insert(addr); // 記錄 victim 地址
            return faultValue_; // 回傳 fault 值
        } else {
            // 如果 detectionRecord_.syndrome 沒有 opIdx 這個 key，則新增
            if (detectionRecord_.syndrome.find(opIdx) == detectionRecord_.syndrome.end()) {
                detectionRecord_.syndrome[opIdx] = false; // 未偵測到
            }
            return mem_[addr]; // 回傳預期值
        }
    }

    if (mem_[addr] != op.value) {
        // 如果讀取的值與預期不符，則記錄偵測結果
        isDetected_ = true;
        detectionRecord_.syndrome[opIdx] = true; // 偵測到 fault
        detectionRecord_.vicAddrs.insert(addr); // 記錄 victim 地址
    } else {
        // 如果 detectionRecord_.syndrome 沒有 opIdx 這個 key，則新增
        if (detectionRecord_.syndrome.find(opIdx) == detectionRecord_.syndrome.end()) {
            detectionRecord_.syndrome[opIdx] = false; // 未偵測到
        }
    }

    return mem_[addr]; // 回傳預期值
}

void TwoCellFault::onWrite(int addr, const SingleOp& op) {
    if (addr != vic_ && addr != aggr_) {
        std::__throw_invalid_argument("Address does not match victim or aggressor cell address.");
    }
    if (type_ == TwoCellFaultType::Saa && addr == aggr_) { 
        if (history_.size() >= trigger_.size()) {
            history_.pop_front(); // 保持歷史記錄長度
        }
        history_.push_back({mem_[addr], op});
    } else if (type_ == TwoCellFaultType::Svv && addr == vic_) {
        if (history_.size() >= trigger_.size()) {
            history_.pop_front(); // 保持歷史記錄長度
        }
        history_.push_back({mem_[addr], op});
    }
    // 記錄寫入操作
    mem_[addr] = op.value;
    if (((type_ == TwoCellFaultType::Saa && addr == aggr_) || (type_ == TwoCellFaultType::Svv && addr == vic_))
        && triggerMatcher()) {
        mem_[addr] = faultValue_; // 注入 fault 值
    }
}

bool TwoCellFault::triggerMatcher() {
    if (trigger_.size() != history_.size()) {
        return false; // 如果歷史記錄長度不等於觸發條件序列長度，則不匹配
    }
    if (type_ == TwoCellFaultType::Saa && mem_[vic_] != coupleTriggerValue_) {
        return false; // Saa 類型需要 coupleTriggerValue 匹配
    }
    if (type_ == TwoCellFaultType::Svv && mem_[aggr_] != coupleTriggerValue_) {
        return false; // Svv 類型需要 coupleTriggerValue 匹配
    }
    return std::equal(trigger_.begin(), trigger_.end(), history_.begin(),
        [](const OperationRecord& a, const OperationRecord& b) {
            return a == b; // 比較操作前的值和操作類型
        });
}

void TwoCellFault::setTrigger(const int AI, const int VI, const std::vector<SingleOp>& seq) {
    trigger_.clear();
    if (type_ == TwoCellFaultType::Saa) {
        // Saa 類型的觸發條件
        coupleTriggerValue_ = VI; // 設定 couple trigger value 為 victim cell 的值
    } else if (type_ == TwoCellFaultType::Svv) {
        // Svv 類型的觸發條件
        coupleTriggerValue_ = AI; // 設定 couple trigger value 為 aggressor cell 的值
    }
    // 設定觸發條件序列   
    for (int i = 0; i < seq.size(); ++i) {
        if (i == 0) {
            if (type_ == TwoCellFaultType::Saa) {
                trigger_.push_back({AI, seq[i]});
            } else if (type_ == TwoCellFaultType::Svv) {
                trigger_.push_back({VI, seq[i]});
            }
            continue;
        }
        trigger_.push_back({seq[i - 1].value, seq[i]});
    }
}

std::string TwoCellFault::getTriggerInfo() const {
    std::string seq = std::to_string(trigger_.front().beforeValue);
    for (const auto& rec : trigger_) {
        seq += rec.op.type == OperationType::READ ? "R" : "W";
        seq += std::to_string(rec.op.value);
    }
    std::string finalF = std::to_string(faultValue_);
    std::string finalR = (finalReadValue_ == UNPROCESS ? "-" : std::to_string(finalReadValue_));
    if (type_ == TwoCellFaultType::Saa) {
        return "< " + seq + " ; " + std::to_string(coupleTriggerValue_) + " / " + finalF + " / " + finalR + " >";
    } else if (type_ == TwoCellFaultType::Svv) {
        return "< " + std::to_string(coupleTriggerValue_) + " ; " + seq + " / " + finalF + " / " + finalR + " >";
    }
    return "< Invalid TwoCellFault type >"; // 錯誤處理
}

// -------------------------------
// FaultSimulator implementation
// -------------------------------

void FaultSimulator::runAll(std::vector<MarchElement>& marchSeq,
                            std::unordered_map<FaultID, std::unique_ptr<BaseFault>>& faults) {
    // 初始化所有 fault 的 syndromes
    for(auto& fault : faults) {
        for(int init : {0,1}) {
            simulateSubcase(marchSeq, fault.second);
        }
    }
}

void FaultSimulator::chooseAggVictim(const std::unique_ptr<BaseFault>& fault, int init) {
    if (!fault) {
        std::__throw_invalid_argument("Fault is null.");
    }

    // 判斷 fault 的具體類型
    if (auto oneCellFault = dynamic_cast<OneCellFault*>(fault.get())) {
        // 處理 OneCellFault
        std::uniform_int_distribution<int> dist(0, rows_ * cols_ - 1);
        int victim = dist(rng_);
        oneCellFault->setVictimState(victim, init); // 設定 victim 的初始狀態
    } else if (auto twoCellFault = dynamic_cast<TwoCellFault*>(fault.get())) {
        // 處理 TwoCellFault
        std::uniform_int_distribution<int> dist(0, rows_ * cols_ - 1);
        int victim = dist(rng_);
        int aggressor;

        if (victim / cols_ == 0) {
            // 如果 victim 在第一行，aggressor 只能是左邊
            aggressor = victim - 1;
        } else if (victim % cols_ == 0) {
            // 如果 victim 在第一列，aggressor 只能是上面
            aggressor = victim - cols_;
        } else {
            // 隨機選擇 aggressor 是上面或左邊
            std::uniform_int_distribution<int> choice(0, 1);
            aggressor = (choice(rng_) == 0) ? victim - cols_ : victim - 1;
        }

        twoCellFault->setAggressorState(aggressor, init); // 設定 aggressor 的初始狀態
        twoCellFault->setVictimState(victim, init);             // 設定 victim 的初始狀態
    } else {
        std::__throw_invalid_argument("Unsupported fault type.");
    }
}

void FaultSimulator::simulateSubcase(std::vector<MarchElement>& marchSeq, std::unique_ptr<BaseFault>& fault) {
    const int initValue = 0;

    if (!fault) {
        std::__throw_invalid_argument("Fault is null.");
    }
    if (marchSeq.empty()) {
        std::__throw_invalid_argument("March sequence is empty.");
    }

    chooseAggVictim(fault, initValue);

    // 執行測試序列並收集偵測結果
    for (int elemIdx = 0; elemIdx < marchSeq.size(); ++elemIdx) {
        const auto& elem = marchSeq[elemIdx];
        if (elem.is_asend) {
            for (auto it = fault->getMemory().begin(); it != fault->getMemory().end(); ++it) {
                // 依照 address 由小到大正序執行
                for (int opIdx = 0; opIdx < elem.ops.size(); ++opIdx) {
                    const auto& op = elem.ops[opIdx];
                    MarchOpIdx marchOpIdx{elemIdx, opIdx};
                    if (op.type == OperationType::READ) {
                        fault->onRead(it->first, marchOpIdx, op);
                    } else if (op.type == OperationType::WRITE) {
                        fault->onWrite(it->first, op);
                    }
                }
            }
        } else  {
            for (auto it = fault->getMemory().rbegin(); it != fault->getMemory().rend(); ++it) {
                // 依照 address 由大到小倒序執行
                for (int opIdx = 0; opIdx < elem.ops.size(); ++opIdx) {
                    const auto& op = elem.ops[opIdx];
                    MarchOpIdx marchOpIdx{elemIdx, opIdx};
                    if (op.type == OperationType::READ) {
                        fault->onRead(it->first, marchOpIdx, op);
                    } else if (op.type == OperationType::WRITE) {
                        fault->onWrite(it->first, op);
                    }
                }
            }
        }
    }
}
