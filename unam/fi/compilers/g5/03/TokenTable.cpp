#include "TokenTable.hpp"

#include <iterator>      // std::size, std::begin, std::end
#include <unordered_set>

// Mantenemos privado todo el bloque de código (encapsulacion)
namespace {

    // Palabras clave
constexpr const char* KEYWORDS[] = {
    "int", "float", "double", "char", "void",
    "if", "else", "while", "for", "do",
    "return", "break", "continue",
    "print", "printf", "scanf",
    "struct", "const", "static", "sizeof",
};

    // Operadores
constexpr const char* OPERATORS[] = {
    // De dos caracteres
    "==", "!=", "<=", ">=", "&&", "||",
    "+=", "-=", "*=", "/=", "%=",
    // De un carácter
    "=", "<", ">", "+", "-", "*", "/", "%", "!", "&", "|",
};

// Puntuación
constexpr char PUNCTUATION_CHARS[] = "(){}[];,:";
constexpr std::size_t PUNCTUATION_COUNT = sizeof(PUNCTUATION_CHARS) - 1; // sin el '\0'

// Total de lexemas distintos soportados. 
constexpr std::size_t TOTAL_LEXEMES =
    std::size(KEYWORDS) + std::size(OPERATORS) + PUNCTUATION_COUNT;

// La rúbrica exige al menos 40 tokens distintos. 
// Usamos assert debido a que no depende de las entradas del usuario
static_assert(TOTAL_LEXEMES >= 40, "Mínimo 40 tokens distintos");

// Busqueda con tablas hash
const std::unordered_set<std::string>& keywords() {
    static const std::unordered_set<std::string> set(std::begin(KEYWORDS),
                                                     std::end(KEYWORDS));
    return set;
}

const std::unordered_set<std::string>& operators() {
    static const std::unordered_set<std::string> set(std::begin(OPERATORS),
                                                     std::end(OPERATORS));
    return set;
}

// Los caracteres que pueden formar un operador se derivan de la tabla de
// operadores. Evita desincronización si se agregan operadores nuevos.
const std::string& operatorChars() {
    static const std::string chars = [] {
        std::string result;
        for (const char* op : OPERATORS) {
            for (const char* p = op; *p != '\0'; ++p) {
                if (result.find(*p) == std::string::npos) {
                    result += *p;
                }
            }
        }
        return result;
    }();
    return chars;
}

const std::string& punctuationChars() {
    static const std::string chars(PUNCTUATION_CHARS);
    return chars;
}

} 

namespace TokenTable {

std::optional<TokenType> lookupKeyword(const std::string& lexeme) {
    if (keywords().count(lexeme) > 0) {
        return TokenType::Keyword;
    }
    return std::nullopt;
}

bool isOperator(const std::string& lexeme) {
    return operators().count(lexeme) > 0;
}

bool isOperatorChar(char c) {
    return operatorChars().find(c) != std::string::npos;
}

bool isPunctuation(char c) {
    return punctuationChars().find(c) != std::string::npos;
}

std::size_t totalLexemes() {
    return TOTAL_LEXEMES;
}

} 
