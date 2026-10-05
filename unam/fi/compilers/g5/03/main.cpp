#include <iostream>
#include <string>

#include "Lexer.hpp"

int main() {
    // 1. Usamos uno de los ejemplos obligatorios de la tarea
    std::cout << "Ingresa el codigo a analizar (ej. printf(\"This is an example\");): \n";

    std::string sourceCode;
    std::getline(std::cin, sourceCode);

    // 2. Toda la lógica y el formato viven en la librería (runLexer).
    //    Así el CLI y la web producen exactamente la misma salida.
    std::cout << runLexer(sourceCode);

    return 0;
}
