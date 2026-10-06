#include <iostream>
#include <string>

#include "Lexer.hpp"

// Modos de salida:
//   (por defecto)  tabla legible: numero, lexema y tipo de cada token
//   --rubric / -r  salida plana EXACTA que pide la rubrica
//
// El prompt va a stderr para que stdout quede limpio: solo el resultado.
int main(int argc, char** argv) {
    bool rubricMode = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--rubric" || arg == "-r") {
            rubricMode = true;
        }
    }

    std::cerr << "Ingresa el codigo a analizar (ej. printf(\"This is an example\");): \n";

    std::string sourceCode;
    std::getline(std::cin, sourceCode);

    // Toda la logica y el formato viven en la libreria (scan/formatTable/
    // runLexer), asi el CLI y la web producen la misma salida.
    if (rubricMode) {
        std::cout << runLexer(sourceCode);
    } else {
        std::cout << formatTable(scan(sourceCode));
    }

    return 0;
}
