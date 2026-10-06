#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "Token.hpp"

namespace TokenTable {

std::optional<TokenType> lookupKeyword(const std::string& lexeme);

// p. ej. "==" 
bool isOperator(const std::string& lexeme);

// p. ej. '+', '='
bool isOperatorChar(char c);

// p. ej. '(', ';'
bool isPunctuation(char c);

// Número total de lexemas distintos soportados.
std::size_t totalLexemes();

} 
