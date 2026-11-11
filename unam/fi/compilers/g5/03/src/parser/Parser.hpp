#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../Token.hpp"
#include "Grammar.hpp"
#include "ParseTree.hpp"

// Una fila de la traza: así estaban la pila y la entrada, y qué se hizo
struct TraceStep {
    std::string stack;
    std::string input;
    std::string action;
};

struct ParseResult {
    bool ok = false;
    std::unique_ptr<Node> tree; // Si hubo error queda incompleto (nodos pendientes)
    int errorLine = 0;
    std::string errorMessage;
    std::vector<TraceStep> trace;
};

// Terminal de la gramática que le corresponde a un token del lexer.
// Keywords, operadores y puntuación usan su lexema; identificadores y
// constantes se agrupan en id, int_const, float_const, char_const y string_lit.
std::string terminalFor(const Token& token);

// Parser predictivo no recursivo: una pila, la tabla M y un token de lookahead.
class Parser {
public:
    explicit Parser(const Grammar& grammar) : grammar(grammar) {}

    ParseResult parse(const std::vector<Token>& tokens, bool withTrace = false) const;

private:
    const Grammar& grammar;
};
