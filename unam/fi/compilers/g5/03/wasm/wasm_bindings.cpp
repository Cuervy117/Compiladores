#include <emscripten/bind.h>

#include <string>

#include "Lexer.hpp"

std::string analyzeTable(const std::string& source) {
    return formatTable(scan(source));
}

EMSCRIPTEN_BINDINGS(lexer) {
    emscripten::function("analyzeTable", &analyzeTable);
    emscripten::function("analyzeRubric", &runLexer);
}
