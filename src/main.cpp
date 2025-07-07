#include "../include/Parser.hpp"
#include "../include/FaultSimulator.hpp"

int main(int argc, char* argv[])
{
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <faults.json> <marchTest.json> <detection_report.txt>\n";
        return 1;
    }

    try {
        Parser parser;
        auto faults = parser.parseFaults(argv[1]);
        auto marchTest = parser.parseMarchTest(argv[2]);

        std::cout << "Enter row and column dimensions for the memory (e.g., 4 4): ";
        int rows, cols;
        std::cin >> rows >> cols;
        if (rows <= 0 || cols <= 0) {
            throw std::invalid_argument("Row and column dimensions must be positive integers.");
        }

        OneByOneFaultSimulator faultSim (faults, marchTest, rows, cols, 12345); 
        faultSim.run();
        // Write detection report
        parser.writeDetectionReport(faults, faultSim.getDetectedRate(), argv[3]);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}