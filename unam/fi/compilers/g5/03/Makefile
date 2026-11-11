# Algunas versiones de mingw32-make (WinLibs) traen como CXX por defecto la ruta
# de la máquina donde se compilaron. Si nadie definió CXX, usamos g++.
ifeq ($(origin CXX),default)
CXX = g++
endif
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic

LEXER_SRC  = src/lexer.cpp src/Token.cpp src/TokenTable.cpp
PARSER_SRC = src/parser/Grammar.cpp src/parser/CGrammar.cpp src/parser/ParseTree.cpp \
             src/parser/Parser.cpp src/semantic/SymbolTable.cpp src/semantic/Semantic.cpp \
             src/app/Analysis.cpp

all: build/lexer build/parser

build/lexer: $(LEXER_SRC) src/main.cpp | build
	$(CXX) $(CXXFLAGS) -Isrc $^ -o $@

build/parser: $(LEXER_SRC) $(PARSER_SRC) src/main_parser.cpp | build
	$(CXX) $(CXXFLAGS) -Isrc $^ -o $@

build:
	mkdir -p build

test: build/parser
	bash tests/run_tests.sh

clean:
	rm -rf build

.PHONY: all test clean
