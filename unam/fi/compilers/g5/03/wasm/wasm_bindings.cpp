#include <emscripten/bind.h>

#include <string>

#include "Lexer.hpp"
#include "app/Analysis.hpp"

std::string analyzeTable(const std::string& source) {
    return formatTable(scan(source));
}

// analyze() y grammar() regresan JSON; app.js lo convierte con JSON.parse.
// Las dos funciones del lexer se dejan igual que en la primera entrega.
EMSCRIPTEN_BINDINGS(compiler) {
    emscripten::function("analyze", &analyzeToJson);
    emscripten::function("grammar", &grammarToJson);
    emscripten::function("analyzeTable", &analyzeTable);
    emscripten::function("analyzeRubric", &runLexer);
}
