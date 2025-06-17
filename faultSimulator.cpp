#include "faultSimulator.hpp"

// -------------------------------
// Cell class implementation
// -------------------------------

Cell::Cell()
    : cell_value(0), TriggerValue(-1), maxQueue(0), faultValue(-1), finalReadValue(-1) {
}

void Cell::init(int init) {
    // 初始化 cell value
    cell_value = init;
    TriggerValue = -1; // 初始無觸發值
    faultValue = -1; // 初始無 fault 觸發值
    finalReadValue = -1; // 初始無讀取最終值
    maxQueue = 0; // 初始 queue 大小為 0
    clearQueue(); // 清空歷史記錄
}

void Cell::installFault(const std::vector<SingleOp>& seq, int TriggerI, int finalF, int finalR) {
    // 安裝 fault sequence to this cell
    // seq 非空時，代表需依序匹配 seq 才觸發 Fault
    // faultValue 保持 -1
    faultSeq = seq;               
    TriggerValue = TriggerI;
    maxQueue = seq.size(); // 設定 queue 大小            
    clearQueue();
    // 設定fault觸發值
    faultValue = finalF; // 觸發 Fault 時的最終值
    finalReadValue = finalR; // 讀取時的最終值                 
}

void Cell::installFault(int TriggerI, int finalF, int finalR) {
    // 安裝單一值觸發 (no sequence)
    // seq 為空，僅比對 cell.value == faultValue
    faultSeq.clear();             
    TriggerValue = TriggerI;
    maxQueue = 0; // 不需要 queue 比對          
    clearQueue();     
    // 設定fault觸發值
    faultValue = finalF; // 觸發 Fault 時的最終值
    finalReadValue = finalR; // 讀取時的最終值               
}

void Cell::clearQueue() {
    // 清空 OperationRecord queue
    // 每個 element 結束後呼叫，防止跨 element
    history.clear();
}

void Cell::recordOperation(const SingleOp& op, int before) {
    // Push new OperationRecord to history
    OperationRecord rec;
    rec.beforeValue = before;
    rec.op = op;
    rec.afterValue = cell_value; // current value after op
    history.push_back(rec);
    if(history.size() > maxQueue) history.pop_front();
}

bool Cell::historyMatches() const {
    if(history.size() < faultSeq.size()) return false;
    // 從 queue.front() 到 back()
    auto it = history.begin();
    if (it->beforeValue != TriggerValue) {
        return false; // 初始值不匹配
    }
    // 檢查歷史是否與 faultSeq 完全匹配
    for(size_t i=0; i<faultSeq.size(); ++i, ++it) {
        if(it->op.type != faultSeq[i].type || it->op.value != faultSeq[i].value)
            return false;
    }
    return true;
}

bool Cell::checkTrigger() {
    if(!faultSeq.empty() && TriggerValue == -1) {
        std::cout << "Error: It is an illegal set." << std::endl;
        return false;
    }
    if(faultSeq.empty() && TriggerValue == -1) {
        return false;
    }
    // 空 seq 但 faultValue != -1 時，直接比對 current value
    if(faultSeq.empty() && TriggerValue != -1) {
        if(cell_value == TriggerValue) {
            return true;
        }
        return false;
    }
    // 否則檢查 history 是否與 faultSeq 完全匹配
    if(historyMatches()) {
        return true;
    }
    return false;
}

void Cell::triggerFault() {
    if (faultValue == -1) {
        return; // 非victim cell 或未設定 faultValue
    }
    // 觸發 Fault: 強制將 cell.value 改為 finalF
    cell_value = faultValue; // faultFinalValue 需另行存入
}

int Cell::applyOp(const SingleOp& op) {
    int before = cell_value;
    if(op.type == OpType::WRITE) {
        cell_value = op.value; // 寫入操作直接更新 value
    }
    // 記錄並嘗試觸發
    recordOperation(op, before);
    if(checkTrigger()) {
        triggerFault();
        if(op.type == OpType::READ) {
            // 如果是讀取操作，則返回觸發後的值
            return finalReadValue != -1 ? finalReadValue : cell_value;
        }
    }
    if (op.type == OpType::READ) {
        // 如果是讀取操作，則返回當前值
        return cell_value;
    }
    // 如果是寫入操作，則不返回值
    return -1; // 寫入操作不返回值
}


// -------------------------------
// Memory class implementation
// -------------------------------

Memory::Memory(int init) {
    // 初始化所有 cell
    for(int i = 0; i < MAX_ADDR; ++i) {
        cells[i].init(init);
    }
}

void Memory::installFault(const FaultSubcase* f, int vicAddr, int aggrAddr) {
    // 對 victim 安裝 seqV 或 value
    if(!f->seqV.empty()) {
        cells[vicAddr].installFault(f->seqV, f->VI, f->finalF, f->finalR);
    } else {
        cells[vicAddr].installFault(f->VI, f->finalF, f->finalR);
    }
    // coupling: 同理對 aggressor
    if(f->type == FaultType::COUPLING) {
        if(!f->seqA.empty()) {
            cells[aggrAddr].installFault(f->seqA, f->VI, -1, -1);
        } else {
            cells[aggrAddr].installFault(f->AI, -1, -1);
        }
    }
}

void Memory::clearElementQueues() {
    // 每個 element 執行完後清除歷史
    for(auto& c : cells) c.clearQueue();
}

void Memory::writeCell(int addr, int v) {
    cells[addr].applyOp({OpType::WRITE, v});
}

int Memory::readCell(int addr, int v) {
    return cells[addr].applyOp({OpType::READ, v});
}

// -------------------------------
// FaultSimulator implementation
// -------------------------------

FaultSimulator::FaultSimulator(const std::vector<FaultPrimitive>& f)
    : faults(f), rng(std::random_device{}()) {
}

void FaultSimulator::runAll() {
    // 初始化所有 fault 的 syndromes
    All_syndromes.clear();
    for(const auto& fp : faults) {
        std::vector<SubcaseSynd> subcaseSynd_vec;
        for(const auto& sc : fp.subs) {
            for(int init : {0,1}) {
                simulateSubcase(subcaseSynd_vec, sc, init);
            }
        }
        // 將每個 fault primitive 的 syndromes 存入 All_syndromes
        All_syndromes[fp.name] = subcaseSynd_vec;
    }
}

std::pair<int,int> FaultSimulator::chooseAggVictim(const FaultSubcase& sc) {
    // 隨機選擇 victim/agg obey sc.A
    // ... implementation ...
    int side = static_cast<int>(sqrt(MAX_ADDR));
    if (sc.type == FaultType::SINGLE) {
        // 單一 fault: 沒有 aggressor
        std::uniform_int_distribution<int> dist(0, MAX_ADDR - 1);
        int vic = dist(rng);
        return {-1, vic}; // -1 表示沒有 aggressor
    }
    if (sc.A == 0) {
        // aggressor < victim
        // 隨機選擇一個合法的 victim
        std::uniform_int_distribution<int> dist(1, MAX_ADDR - 1);
        int vic = dist(rng);
        if (vic / side == 0) {
            // 如果是第一行，則不能選擇上面
            return {vic - 1, vic}; // 左邊
        } else if (vic % side == 0) {
            // 如果是第一列，則不能選擇左邊
            return {vic - side, vic}; // 上面
        } else {
            // 隨機選擇上面或左邊
            std::uniform_int_distribution<int> choice(0, 1);
            if (choice(rng) == 0) {
                return {vic - side, vic}; // 上面
            } else {
                return {vic - 1, vic}; // 左邊
            }
        }
    }
    if (sc.A == 1) {
        // aggressor > victim
        // 隨機選擇一個合法的 victim
        std::uniform_int_distribution<int> dist(0, MAX_ADDR - 2);
        int vic = dist(rng);
        if (vic / side == side - 1) {
            // 如果是最後一行，則不能選擇下面
            return {vic + 1, vic}; // 右邊
        } else if (vic % side == side - 1) {
            // 如果是最後一列，則不能選擇右邊
            return {vic + side, vic}; // 下面
        } else {
            // 隨機選擇下面或右邊
            std::uniform_int_distribution<int> choice(0, 1);
            if (choice(rng) == 0) {
                return {vic + side, vic}; // 下面
            } else {
                return {vic + 1, vic}; // 右邊
            }
        }
    }
    std::cout << "Error: Invalid FaultSubcase A value: " << sc.A << std::endl;
    return {-1, -1}; // 不應該到這裡
}

void FaultSimulator::simulateSubcase(std::vector<SubcaseSynd>& subcaseSynd_vec, const FaultSubcase& sc, int init) {
    Memory mem(init);
    auto [aggr, vic] = chooseAggVictim(sc);
    mem.installFault(&sc, vic, aggr);
    SubcaseSynd subcaseSynd;
    for(const auto& elem : marchSeq) {
        for(const auto& addr : iterateAddresses(elem.addr_order)) {
            for(const auto& op : elem.ops) {
                if(op.type==OpType::WRITE) {
                    mem.writeCell(addr, op.value);
                } else if(op.type==OpType::READ) {
                    auto it = subcaseSynd.find(op.order);
                    if (it == subcaseSynd.end()) {
                        // 如果是第一次讀取，則初始化 syndrome
                        OpSynd s;
                        subcaseSynd[op.order] = s;
                    }
                    int readValue = mem.readCell(addr, op.value);
                    // 檢查是否有 fault 觸發
                    if (readValue != op.value) {
                        // Fault detected
                        subcaseSynd[op.order].detected = true;
                        subcaseSynd[op.order].addresses.push_back(addr);
                    }
                }
            }
        }
        mem.clearElementQueues();
    }
    subcaseSynd_vec.push_back(subcaseSynd);
}

std::vector<int> FaultSimulator::iterateAddresses(bool addr_order) {
    std::vector<int> addrs;
    if (!addr_order) {
        for (int i = 0; i < MAX_ADDR; ++i) {
            addrs.push_back(i);
        }
    } else {
        for (int i = MAX_ADDR - 1; i >= 0; --i) {
            addrs.push_back(i);
        }
    }
    return addrs;
}
