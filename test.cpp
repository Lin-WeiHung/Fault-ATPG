#include <iostream>
#include <vector>
#include <map>
#include <string>
#include "faultSimulator.hpp"

class FaultSystem {
private:
    OneCellFault* fault_ = new OneCellFault({"TestFault", 0}); // 使用者定義的 Fault
    int victimCell, initValue, triggerValue, faultValue, finalReadValue;
    std::vector<SingleOp> triggerSeq;

public:
    void defineFault() {
        fault_->reset(); // 重置 Fault 狀態
        std::cout << "定義 Fault:\n";
        std::cout << "Victim Cell addr: ";
        std::cin >> victimCell;
        std::cout << "初始化值: ";
        std::cin >> initValue;
        fault_->setVictimState(victimCell, initValue);
        std::cout << "觸發值: ";
        std::cin >> triggerValue;

        int numOps;
        std::cout << "觸發序列操作數量: ";
        std::cin >> numOps;
        for (int i = 0; i < numOps; ++i) {
            char type;
            int value;
            std::cout << "操作類型 (r: READ, w: WRITE): ";
            std::cin >> type;
            std::cout << "操作值: ";
            std::cin >> value;
            triggerSeq.push_back({type == 'r' ? OperationType::READ : OperationType::WRITE, value});
        }
        fault_->setTrigger(triggerValue, triggerSeq);
        std::cout << "Fault Value: ";
        std::cin >> faultValue;
        fault_->setFaultValue(faultValue);
        std::cout << "最終讀取值: ";
        std::cin >> finalReadValue;
        fault_->setFinalReadValue(finalReadValue);

        std::cout << "Fault 定義完成！\n";
    }

    void testOperations() {
        while (true) {
            std::cout << "\n目前記憶體狀況:\n";
            for (const auto& [addr, value] : fault_->getMemory()) {
                std::cout << "Cell " << addr << ": " << value << "\n";
            }

            std::cout << "\n目前 Fault 的觸發條件: " << triggerValue;
            for (const auto& op : triggerSeq) {
                std::cout << (op.type == OperationType::READ ? "R" : "W") << op.value;
            }
            std::cout << "\n";

            int addr, value;
            char type;
            std::cout << "\n輸入操作 (格式: addr type value, type: r/w): ";
            if (!(std::cin >> addr >> type >> value)) {
                std::cout << "輸入格式錯誤，請重新輸入。\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }

            SingleOp op = {type == 'r' ? OperationType::READ : OperationType::WRITE, value};

            if (type == 'w') fault_->onWrite(addr, op);
            if (type == 'r') fault_->onRead(addr, {}, op);

            if (fault_->triggerMatcher()) {
                std::cout << "TestFault 已觸發！\n";
            }
        }
    }
};

int main() {
    FaultSystem system;

    std::cout << "歡迎使用 Fault 測試系統！\n";
    system.defineFault(); // 定義 Fault
    system.testOperations(); // 測試操作

    return 0;
}