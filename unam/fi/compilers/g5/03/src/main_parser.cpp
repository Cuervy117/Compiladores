#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Lexer.hpp"
#include "app/Analysis.hpp"
#include "parser/CGrammar.hpp"
#include "parser/Parser.hpp"
#include "semantic/Semantic.hpp"

// Códigos de salida: 0 todo bien, 1 error de sintaxis, 2 error del SDT, 3 uso/archivo
namespace {

struct Options {
    std::string file;
    std::string treeName; // --tree <nombre>  genera <nombre>.mmd y <nombre>.html
    bool compact = false;
    bool mermaid = false;
    bool trace = false;
    bool tokens = false;
    bool grammar = false;
    bool symbols = false;
    bool json = false;
};

void usage() {
    std::cerr <<
        "Uso: parser [archivo.c] [opciones]\n"
        "  --tokens        muestra los tokens que entrega el lexer\n"
        "  --trace         muestra la pila, la entrada y la accion en cada paso\n"
        "  --grammar       imprime la gramatica, FIRST, FOLLOW y la tabla LL(1)\n"
        "  --tree <nombre> guarda el parse tree en <nombre>.mmd y <nombre>.html\n"
        "  --mermaid       imprime el codigo Mermaid del arbol en la salida\n"
        "  --compact       arbol sin ramas vacias (ε) y con cadenas de un hijo juntas\n"
        "  --symbols       muestra la tabla de simbolos que llena el SDT\n"
        "  --json          todo el analisis en JSON (lo mismo que usa la pagina web)\n"
        "Sin archivo se lee de la entrada estandar.\n";
}

bool readOptions(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--tokens") opt.tokens = true;
        else if (arg == "--trace") opt.trace = true;
        else if (arg == "--grammar") opt.grammar = true;
        else if (arg == "--mermaid") opt.mermaid = true;
        else if (arg == "--compact") opt.compact = true;
        else if (arg == "--symbols") opt.symbols = true;
        else if (arg == "--json") opt.json = true;
        else if (arg == "--tree" && i + 1 < argc) opt.treeName = argv[++i];
        else if (arg == "-h" || arg == "--help") return false;
        else if (!arg.empty() && arg[0] != '-' && opt.file.empty()) opt.file = arg;
        else return false;
    }
    return true;
}

std::string readAll(std::istream& in) {
    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string text = buffer.str();
    if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) text.erase(0, 3); // BOM de UTF-8
    return text;
}

std::string fit(const std::string& s, std::size_t width, bool keepEnd) {
    if (s.size() <= width) return s + std::string(width - s.size(), ' ');
    if (keepEnd) return "..." + s.substr(s.size() - (width - 3));
    return s.substr(0, width - 3) + "...";
}

void printTokens(const std::vector<Token>& tokens) {
    std::cout << "LINE  LEXEME                TERMINAL\n";
    for (const Token& t : tokens) {
        std::cout << fit(std::to_string(t.line), 6, false) << fit(t.value, 22, false) << terminalFor(t) << "\n";
    }
    std::cout << "\n";
}

// La pila se recorta por la izquierda (lo importante es el tope) y la
// entrada por la derecha (lo importante es el lookahead).
void printTrace(const std::vector<TraceStep>& steps) {
    const std::size_t stackWidth = 56;
    const std::size_t inputWidth = 36;
    std::cout << fit("STACK", stackWidth, false) << "  " << fit("INPUT", inputWidth, false) << "  ACTION\n";
    for (const TraceStep& s : steps) {
        std::cout << fit(s.stack, stackWidth, true) << "  " << fit(s.input, inputWidth, false) << "  " << s.action << "\n";
    }
    std::cout << "\n";
}

void printSymbols(const std::vector<SymbolRecord>& log) {
    std::cout << "\nSYMBOL TABLE\n";
    std::cout << fit("NAME", 14, false) << fit("KIND", 11, false) << fit("TYPE", 8, false)
              << fit("PARAMS", 16, false) << fit("LINE", 6, false) << "SCOPE\n";
    for (const SymbolRecord& s : log) {
        std::cout << fit(s.name, 14, false) << fit(s.kind, 11, false) << fit(s.type, 8, false)
                  << fit(s.params.empty() ? "-" : s.params, 16, false) << fit(std::to_string(s.line), 6, false)
                  << s.scope << "\n";
    }
    if (log.empty()) std::cout << "(empty)\n";
}

bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out << content;
    return static_cast<bool>(out);
}

}

int main(int argc, char** argv) {
    Options opt;
    if (!readOptions(argc, argv, opt)) {
        usage();
        return 3;
    }

    const Grammar* grammar = nullptr;
    try {
        grammar = &cGrammar();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 3;
    }

    if (opt.grammar) {
        std::cout << (opt.json ? grammarToJson() + "\n" : grammar->report());
        return 0;
    }

    std::string source;
    if (opt.file.empty()) {
        std::cerr << "Ingresa el codigo a analizar (Ctrl+D para terminar, Ctrl+Z y Enter en Windows):\n";
        source = readAll(std::cin);
    } else {
        std::ifstream in(opt.file, std::ios::binary);
        if (!in) {
            std::cerr << "No se pudo abrir " << opt.file << "\n";
            return 3;
        }
        source = readAll(in);
    }

    if (opt.json) {
        std::cout << analyzeToJson(source) << "\n";
        return 0;
    }

    const std::vector<Token> tokens = scan(source);
    if (opt.tokens) printTokens(tokens);

    const Parser parser(*grammar);
    const ParseResult result = parser.parse(tokens, opt.trace);
    if (opt.trace) printTrace(result.trace);

    int status = 0;
    if (!result.ok) {
        std::cout << "Parsing error...\n";
        std::cout << "  error in line " << result.errorLine << ": " << result.errorMessage << "\n";
        std::cout << "SDT error...\n";
        std::cout << "  the SDT rules were not evaluated because the parse tree is incomplete\n";
        status = 1;
    } else {
        std::cout << "Parsing Success!\n";
        Semantic sdt;
        if (sdt.check(result.tree.get())) {
            std::cout << "SDT Verified!\n";
        } else {
            std::cout << "SDT error...\n";
            for (const Diagnostic& d : sdt.errors()) {
                std::cout << "  error in line " << d.line << ": " << d.message << "\n";
            }
            status = 2;
        }
        for (const Diagnostic& d : sdt.warnings()) {
            std::cout << "  warning in line " << d.line << ": " << d.message << "\n";
        }
        if (opt.symbols) printSymbols(sdt.symbolLog());
    }

    if (opt.mermaid || !opt.treeName.empty()) {
        const std::string diagram = toMermaid(result.tree.get(), opt.compact);
        if (opt.mermaid) std::cout << "\n" << diagram;

        if (!opt.treeName.empty()) {
            const std::string title = "Parse tree: " + (opt.file.empty() ? std::string("stdin") : opt.file);
            const bool ok = writeFile(opt.treeName + ".mmd", diagram) &&
                            writeFile(opt.treeName + ".html", mermaidPage(diagram, title));
            if (ok) {
                std::cerr << "Arbol guardado en " << opt.treeName << ".mmd y " << opt.treeName << ".html\n";
            } else {
                std::cerr << "No se pudo escribir " << opt.treeName << ".mmd/.html\n";
            }
        }
    }

    return status;
}
