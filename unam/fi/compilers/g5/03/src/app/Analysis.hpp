#pragma once

#include <string>

// Une las tres fases (lexer -> parser -> SDT) y regresa todo en JSON.
// La página web llama a estas funciones desde WebAssembly y el CLI las
// usa con --json, así las dos salidas siempre son las mismas.

// Tokens, conteos, resultado del parser y del SDT, tabla de símbolos,
// parse tree en Mermaid (completo y compacto) y la traza del LL(1).
std::string analyzeToJson(const std::string& source);

// Producciones, FIRST, FOLLOW, tabla LL(1) y conflictos. No depende de la entrada.
std::string grammarToJson();
