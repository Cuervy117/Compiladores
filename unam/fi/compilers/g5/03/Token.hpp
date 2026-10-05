#pragma once

#include <string>

// Categorías exactas que se piden en la rúbrica
enum class TokenType {
    Keyword,     // Para "printf" o "int"
    Identifier,  // Para variables como "$a"
    Operator,    // Para símbolos como "=", "+", "-"
    Constant,    // Para números como "10" o literales de texto
    Punctuation, // Para "(", ")", ";", etc.
    EndOfFile,   // Para saber cuándo terminamos de leer
    Unknown      // Por si encontramos un carácter no válido
};

// Esta estructura guardará el tipo de token y su texto original (lexema)
struct Token {
    TokenType type;
    std::string value;
};

// Traduce el tipo de token a la etiqueta de salida ("keyword", "identifier", ...).
// Devuelve "" para EndOfFile y Unknown (no se imprimen).
std::string getCategoryName(TokenType type);
