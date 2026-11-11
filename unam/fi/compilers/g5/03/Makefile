ifeq ($(origin CXX),default)
CXX = g++
endif
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic

LEXER_SRC = src/lexer.cpp src/Token.cpp src/TokenTable.cpp

all: build/lexer

build/lexer : $(LEXER_SRC) src/main.cpp | build
	$(CXX) $(CXXFLAGS) -Isrc $^ -o $@

build:
	mkdir -p build

clean: 
	rm -rf build

.PHONY: all clean
