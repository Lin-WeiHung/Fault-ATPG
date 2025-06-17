CXX = g++
CXXFLAGS = -g -std=c++17

OUTPUT_LIST = main

com: faultSimulator.cpp parser.cpp MarchGenerator.cpp main.cpp
	$(CXX) $(CXXFLAGS) -o main faultSimulator.cpp parser.cpp MarchGenerator.cpp main.cpp

clean:
	rm -f $(OUTPUT_LIST)