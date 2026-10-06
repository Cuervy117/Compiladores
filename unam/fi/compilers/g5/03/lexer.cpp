#include "Lexer.hpp"
#include "TokenTable.hpp"

#include <cctype> // isalpha(), isdigit(), isspace(), isalnum()

Lexer::Lexer(const std::string& sourceCode) : source(sourceCode) {}

char Lexer::peek() const {
    if (pos >= source.length()) return '\0'; // '\0' = llegamos al final
    return source[pos];
}

char Lexer::advance() {
    if (pos >= source.length()) return '\0';
    return source[pos++];
}

void Lexer::skipWhitespace() {
    while (std::isspace(peek())) {
        advance();
    }
}

Token Lexer::getNextToken() {
    skipWhitespace(); // Ignoramos los espacios en blanco
    char c = peek();

    // 1. Fin de archivo
    if (c == '\0') {
        return {TokenType::EndOfFile, ""};
    }

    // 2. Puntuación: siempre un solo carácter, se consulta la tabla
    if (TokenTable::isPunctuation(c)) {
        return {TokenType::Punctuation, std::string(1, advance())};
    }

    // 3. Operadores: pueden ser de uno o dos caracteres (=, ==, +=, ...)
    if (TokenTable::isOperatorChar(c)) {
        return scanOperator();
    }

    // 4. Constantes de texto (String literals como "This is an example")
    if (c == '"') {
        std::string str = "";
        str += advance(); // Consumimos la primera comilla

        while (peek() != '"' && peek() != '\0') {
            str += advance(); // Guardamos el texto interior
        }

        if (peek() == '"') {
            str += advance(); // Consumimos la última comilla
        }
        return {TokenType::Constant, str};
    }

    // 5. Constantes numéricas (como el 10)
    if (std::isdigit(c)) {
        std::string num = "";
        while (std::isdigit(peek())) {
            num += advance();
        }
        return {TokenType::Constant, num};
    }

    // 6. Palabras clave (Keywords) e Identificadores (Variables como $a)
    if (std::isalpha(c) || c == '$') {
        std::string text = "";

        // Seguimos leyendo mientras sean letras, números o el símbolo $
        while (std::isalnum(peek()) || peek() == '$') {
            text += advance();
        }

        if (auto keyword = TokenTable::lookupKeyword(text)) {
            return {*keyword, text};
        }

        // Si no es palabra clave, es un identificador
        return {TokenType::Identifier, text};
    }

    // 7. Si llegamos aquí, es un carácter que no conocemos
    return {TokenType::Unknown, std::string(1, advance())};
}

// Lee una racha de caracteres de operador
// gana el prefijo más largo que exista en la tabla ("==" antes que "="),
// y los caracteres sobrantes se devuelven al flujo.
Token Lexer::scanOperator() {
    std::string run;
    while (TokenTable::isOperatorChar(peek())) {
        run += advance();
    }

    for (std::size_t len = run.size(); len >= 1; --len) {
        std::string candidate = run.substr(0, len);
        if (TokenTable::isOperator(candidate)) {
            pos -= (run.size() - len); // devolvemos los caracteres sobrantes
            return {TokenType::Operator, candidate};
        }
    }

    // Ningún prefijo es un operador conocido: reportamos solo el primero
    pos -= (run.size() - 1);
    return {TokenType::Unknown, std::string(1, run[0])};
}

std::vector<Token> scan(const std::string& source) {
    Lexer lexer(source);
    std::vector<Token> tokens;

    Token token = lexer.getNextToken();
    while (token.type != TokenType::EndOfFile) {
        tokens.push_back(token);
        token = lexer.getNextToken();
    }

    return tokens;
}

std::string formatRubric(const std::vector<Token>& tokens) {
    std::string outputSequence = "";
    int tokenCount = 0;

    for (const Token& token : tokens) {
        // Filtramos los desconocidos, igual que antes
        if (token.type != TokenType::Unknown) {
            outputSequence += getCategoryName(token.type) + " ";
            tokenCount++;
        }
    }

    outputSequence += "\n";
    outputSequence += "Total of tokens: " + std::to_string(tokenCount) + "\n";
    return outputSequence;
}

std::string runLexer(const std::string& source) {
    return formatRubric(scan(source));
}
