CXX = g++
CXXFLAGS = -g -std=c++17

OURPUT_LIST = main

com: faultSimulator.cpp parser.cpp main.cpp
	$(CXX) $(CXXFLAGS) -o main faultSimulator.cpp parser.cpp main.cpp

clean:
	rm -f $(OURPUT_LIST)