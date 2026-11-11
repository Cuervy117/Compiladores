#pragma once

#include <string>
#include <vector>

#include "../parser/ParseTree.hpp"
#include "SymbolTable.hpp"

struct Diagnostic {
    int line;
    std::string message;
};

// Registro de cada símbolo que entró a la tabla, con el ámbito donde vivía.
// La tabla de símbolos real se vacía al cerrar cada ámbito, así que esto es
// lo que se muestra con --symbols y en la página web.
struct SymbolRecord {
    std::string name;
    std::string kind;   // variable, parameter o function
    std::string type;
    std::string params; // solo funciones: "int, int"
    int line = 0;
    std::string scope;  // global, main(), block (line 5), for (line 7)
    int depth = 0;      // 0 = global
};

// Reglas del SDT evaluadas sobre el parse tree ya construido.
//
// Hay un método por no terminal (o grupo de no terminales con la misma forma).
// Los atributos heredados entran como parámetros (p. ej. el tipo de una
// declaración, el operando izquierdo de una cola XTail) y los sintetizados
// salen como valor de retorno (el tipo de cada expresión).
class Semantic {
public:
    // true si no se violó ninguna regla
    bool check(const Node* root);

    const std::vector<Diagnostic>& errors() const { return errorList; }
    const std::vector<Diagnostic>& warnings() const { return warningList; }
    const std::vector<SymbolRecord>& symbolLog() const { return records; }

private:
    SymbolTable symbols;
    std::vector<Diagnostic> errorList;
    std::vector<Diagnostic> warningList;
    std::vector<SymbolRecord> records;
    std::vector<std::string> scopeNames; // nombre de cada ámbito abierto

    // Atributos heredados que bajan por todas las sentencias
    bool insideFunction = false;
    Type returnType;
    int loopDepth = 0;

    void enterScope(const std::string& name);
    void leaveScope();
    void record(const Symbol& s, const std::string& kind);

    void error(int line, const std::string& message);
    void warning(int line, const std::string& message);

    void item(const Node* n);
    void function(Type type, const Node* id, const Node* tail);
    void declarators(Type type, const Node* id, const Node* varTail);
    void variable(Type type, const Node* id, const Node* init);

    void blockItems(const Node* n);
    void block(const Node* n);
    void statement(const Node* n);
    void loopBody(const Node* stmt);
    void forStatement(const Node* n);
    void returnStatement(const Node* n);
    void ioStatement(const Node* n);
    void formatCall(const std::string& name, const std::vector<const Node*>& args, int line);
    void condition(const Node* expr, const std::string& where);

    Type expression(const Node* n);
    Type binary(const Node* n);
    Type binaryTail(const Node* tail, Type inherited);
    Type unary(const Node* n);
    Type primary(const Node* n);
    Type call(const Node* id, const Node* callTail);
    Type operation(const std::string& op, Type left, Type right, int line);

    void checkAssign(Type target, Type value, int line, const std::string& what);
};
