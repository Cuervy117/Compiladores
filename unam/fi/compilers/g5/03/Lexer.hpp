#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Token.hpp"

// Analizador léxico. Recorre el código fuente carácter por carácter
// y devuelve un Token a la vez mediante getNextToken().
class Lexer {
public:
    explicit Lexer(const std::string& sourceCode);

    // Devuelve la siguiente pieza del código. Al terminar reporta EndOfFile.
    Token getNextToken();

private:
    std::string source;  // Código completo a analizar
    std::size_t pos = 0; // Cursor: en qué posición vamos

    char peek() const;   // Mira el carácter actual SIN avanzar
    char advance();      // Lee el carácter actual y AVANZA
    void skipWhitespace();
};

// ---------------------------------------------------------------------------
// API compartida entre el CLI de terminal y la web (vía WebAssembly).
// La lógica de formato vive aquí para que ambas salidas sean idénticas.
// ---------------------------------------------------------------------------

// Recorre el código y devuelve todos los tokens (sin EndOfFile).
std::vector<Token> scan(const std::string& source);

// Genera la salida con el formato exacto de la rúbrica a partir de una lista.
std::string formatRubric(const std::vector<Token>& tokens);

// Atajo: scan() + formatRubric().
std::string runLexer(const std::string& source);
