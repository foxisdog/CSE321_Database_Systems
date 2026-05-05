CXX      = g++
CXXFLAGS = -O2 -std=c++17

all: experiments/experiment

experiments/experiment: experiments/experiment.cc src/btree.cc src/bplus_tree.cc src/bstar_tree.cc src/storage.cc
	$(CXX) $(CXXFLAGS) -o experiments/experiment experiments/experiment.cc

clean:
	rm -f experiments/experiment
	rm -f experiments/result.csv

.PHONY: all clean
