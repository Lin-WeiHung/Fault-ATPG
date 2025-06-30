#include <iostream>
#include <cassert>
#include "../include/MemoryState.hpp"

void testDenseMemoryState() {
    std::cout << "Running DenseMemoryState tests...\n";

    // 初始化 10x10 的記憶體，預設值為 -1
    DenseMemoryState memory(10, 10, -1);

    // 測試預設值
    assert(memory.read(0) == -1);
    assert(memory.read(99) == -1);
    assert(memory.read(100) == -1); // 超出範圍，應回傳預設值

    // 測試寫入和讀取
    memory.write(0, 42);
    assert(memory.read(0) == 42);

    memory.write(99, 99);
    assert(memory.read(99) == 99);

    // 測試超出範圍的寫入
    memory.write(100, 123); // 超出範圍，應忽略
    assert(memory.read(100) == -1);

    // 測試負地址
    assert(memory.read(-1) == -1); // 負地址，應回傳預設值

    std::cout << "All DenseMemoryState tests passed!\n";
}

int main() {
    testDenseMemoryState();
    return 0;
}