#include <iostream>
#include <vector>
#include <chrono>

int main() {
    int opNum;
    std::cout << "Enter the number of operations: ";
    std::cin >> opNum;
    if (opNum <= 0) {
        std::cerr << "Error: Number of operations must be positive." << std::endl;
        return 1;
    }
    std::vector<int> operations(opNum);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < opNum; ++i) {
        operations[i] = 1;
    }
    while (true) {
        // Print current combination
        // for (int i = 0; i < opNum; ++i) {
        //     std::cout << operations[i] << (i == opNum - 1 ? '\n' : ' ');
        // }
        // Generate next combination
        int idx = opNum - 1;
        while (idx >= 0 && operations[idx] == 4) {
            operations[idx] = 1;
            --idx;
        }
        if (idx < 0) break;
        ++operations[idx];
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Elapsed time: " << elapsed.count() << " seconds" << std::endl;
}