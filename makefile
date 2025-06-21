CXX = g++
CXXFLAGS = -g -std=c++17

OUTPUT_LIST = main

com: faultSimulator.cpp parser.cpp main.cpp
	$(CXX) $(CXXFLAGS) -o main faultSimulator.cpp parser.cpp main.cpp

TEST_LIST = faultSimulator.cpp

test: test.cpp 
	$(CXX) $(CXXFLAGS) -o test test.cpp $(TEST_LIST)

clean:
	rm -f $(OUTPUT_LIST)