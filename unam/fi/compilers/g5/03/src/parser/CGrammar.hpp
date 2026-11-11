#pragma once

#include "Grammar.hpp"

// Gramática que acepta el parser.
// Se construye una sola vez.
const Grammar& cGrammar();

// Texto de la gramática 
const char* cGrammarText();
