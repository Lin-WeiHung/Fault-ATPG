CXX = g++
CXXFLAGS = -g -std=c++17

OURPUT_LIST = TimeC_test faultSimulator

TimeC_test: TimeC_test.cpp
	$(CXX) $(CXXFLAGS) -o TimeC_test TimeC_test.cpp

faultSimulator: faultSimulator.cpp faultSimulator.hpp
	$(CXX) $(CXXFLAGS) -o faultSimulator faultSimulator.cpp

clean:
	rm -f $(OURPUT_LIST)