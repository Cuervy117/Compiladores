#include "Token.hpp"

std::string getCategoryName(TokenType type) {
    switch (type) {
        case TokenType::Keyword:     return "keyword";
        case TokenType::Identifier:  return "identifier";
        case TokenType::Operator:    return "operator";
        case TokenType::Constant:    return "constant";
        case TokenType::Punctuation: return "punctuation";
        default:                     return ""; // Ignoramos los desconocidos
    }
}
