CXX = $(shell command -v clang++ || command -v g++ || echo g++)
CXXFLAGS = -Wall -Wextra -pedantic -O3 -std=c++17
INCLUDES := -I../include/

all: disk

disk: disk.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -f disk

.PHONY: all clean
