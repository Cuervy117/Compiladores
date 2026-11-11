#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// Error se usa cuando una subexpresión ya falló: así no se reporta
// el mismo problema varias veces hacia arriba en el árbol.
enum class BaseType { Void, Char, Int, Float, Double, Error };

struct Type {
    BaseType base = BaseType::Error;
    bool pointer = false; // char* para cadenas, T* para &x
};

Type makeType(BaseType base, bool pointer = false);
Type typeFromKeyword(const std::string& keyword);
std::string typeName(Type t);

bool isError(Type t);
bool isNumeric(Type t);   // char, int, float, double
bool isIntegral(Type t);  // char, int
bool isFloating(Type t);  // float, double
bool isScalar(Type t);    // numérico o apuntador (sirve como condición)
int rank(Type t);         // char < int < float < double
bool sameType(Type a, Type b);

struct Symbol {
    std::string name;
    Type type;               // En funciones es el tipo de retorno
    bool isFunction = false;
    std::vector<Type> params;
    int line = 0;
};

// Tabla de símbolos con ámbitos anidados (pila de tablas hash).
class SymbolTable {
public:
    SymbolTable() { openScope(); } // ámbito global

    void openScope();
    void closeScope();
    std::size_t depth() const { return scopes.size(); }

    // false si el nombre ya existe en el ámbito actual
    bool declare(const Symbol& symbol);

    // Busca del ámbito más interno al global
    const Symbol* lookup(const std::string& name) const;
    const Symbol* lookupCurrent(const std::string& name) const;

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
};
