#include <iostream>
#include <string>
#include <vector>
#include <cctype> // Nos servirá para funciones como isalpha(), isdigit() o isspace()

// Definimos las categorías exactas que pide tu profesor
enum class TokenType {
    Keyword,     // Para "printf" o "int"
    Identifier,  // Para variables como "$a"
    Operator,    // Para símbolos como "=", "+", "-"
    Constant,    // Para números como "10"
    Punctuation, // Para "(", ")", ";", etc.
    EndOfFile,   // Para saber cuándo terminamos de leer
    Unknown      // Por si encontramos un carácter no válido
};

// Esta estructura guardará el tipo de token y su texto original (lexema)
struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
private:
    std::string source; // Aquí guardaremos todo el código a analizar
    size_t pos = 0;     // Nuestro "dedo" o cursor que apunta en qué letra vamos

    // 1. Función para mirar el carácter actual SIN avanzar
    char peek() const {
        if (pos >= source.length()) return '\0'; // '\0' significa que llegamos al final
        return source[pos];
    }

    // 2. Función para leer el carácter actual y AVANZAR a la siguiente posición
    char advance() {
        if (pos >= source.length()) return '\0';
        return source[pos++];
    }

    // 3. Función para saltar los espacios vacíos (no nos interesan como tokens)
    void skipWhitespace() {
        while (std::isspace(peek())) {
            advance();
        }
    }

public:
    // Al crear el Lexer, le entregamos el texto que va a procesar
    Lexer(const std::string& sourceCode) : source(sourceCode) {}

    // Esta será la función "estrella" que construiremos en el siguiente paso
    Token getNextToken();
};

Token Lexer::getNextToken() {
    skipWhitespace(); // Ignoramos los espacios en blanco
    char c = peek();

    // 1. Fin de archivo
    if (c == '\0') {
        return {TokenType::EndOfFile, ""};
    }

    // 2. Puntuación (paréntesis y punto y coma)
    if (c == '(' || c == ')' || c == ';') {
        return {TokenType::Punctuation, std::string(1, advance())};
    }

    // 3. Operadores (como el signo de igual en $a=10)
    if (c == '=') {
        return {TokenType::Operator, std::string(1, advance())};
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
    // Nota: Agregamos '$' a la condición para que acepte tus variables
    if (std::isalpha(c) || c == '$') {
        std::string text = "";
        
        // Seguimos leyendo mientras sean letras, números o el símbolo $
        while (std::isalnum(peek()) || peek() == '$') {
            text += advance();
        }
        
        // Verificamos si es una palabra clave reservada
        if (text == "print" || text == "printf" || text == "int") {
            return {TokenType::Keyword, text};
        }
        
        // Si no es palabra clave, es un identificador
        return {TokenType::Identifier, text};
    }

    // 7. Si llegamos aquí, es un carácter que no conocemos
    return {TokenType::Unknown, std::string(1, advance())};
}

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

int main() {
    // 1. Usamos uno de los ejemplos obligatorios de la tarea
    std::string sourceCode;

    std::cout << "Ingresa el codigo a analizar (ej. printf(\"This is an example\");): \n";
    std::getline(std::cin, sourceCode);
    
    
    Lexer lexer(sourceCode);

    // 2. Variables para acumular la salida y contar
    std::string outputSequence = "";
    int tokenCount = 0;

    // 3. Obtenemos la primera pieza
    Token token = lexer.getNextToken();

    // 4. Bucle principal: leemos hasta que nos reporte que se acabó el texto
    while (token.type != TokenType::EndOfFile) {
        
        // Filtramos para evitar sumar espacios vacíos o errores no contemplados
        if (token.type != TokenType::Unknown) { 
            outputSequence += getCategoryName(token.type) + " ";
            tokenCount++;
        }
        
        // Avanzamos a la siguiente pieza
        token = lexer.getNextToken();
    }

    // 5. Imprimimos el resultado con el formato exacto requerido
    std::cout << outputSequence << "\n";
    std::cout << "Total of tokens: " << tokenCount << "\n";

    return 0;
}