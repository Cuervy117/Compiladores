#include "SymbolTable.hpp"

Type makeType(BaseType base, bool pointer) {
    Type t;
    t.base = base;
    t.pointer = pointer;
    return t;
}

Type typeFromKeyword(const std::string& keyword) {
    if (keyword == "int") return makeType(BaseType::Int);
    if (keyword == "float") return makeType(BaseType::Float);
    if (keyword == "double") return makeType(BaseType::Double);
    if (keyword == "char") return makeType(BaseType::Char);
    if (keyword == "void") return makeType(BaseType::Void);
    return makeType(BaseType::Error);
}

std::string typeName(Type t) {
    std::string name;
    switch (t.base) {
        case BaseType::Void:   name = "void"; break;
        case BaseType::Char:   name = "char"; break;
        case BaseType::Int:    name = "int"; break;
        case BaseType::Float:  name = "float"; break;
        case BaseType::Double: name = "double"; break;
        default:               name = "<error>";
    }
    return t.pointer ? name + "*" : name;
}

bool isError(Type t) {
    return t.base == BaseType::Error;
}

bool isNumeric(Type t) {
    return !t.pointer && t.base != BaseType::Void && t.base != BaseType::Error;
}

bool isIntegral(Type t) {
    return !t.pointer && (t.base == BaseType::Char || t.base == BaseType::Int);
}

bool isFloating(Type t) {
    return !t.pointer && (t.base == BaseType::Float || t.base == BaseType::Double);
}

bool isScalar(Type t) {
    return isNumeric(t) || (t.pointer && !isError(t));
}

int rank(Type t) {
    switch (t.base) {
        case BaseType::Char:   return 1;
        case BaseType::Int:    return 2;
        case BaseType::Float:  return 3;
        case BaseType::Double: return 4;
        default:               return 0;
    }
}

bool sameType(Type a, Type b) {
    return a.base == b.base && a.pointer == b.pointer;
}

void SymbolTable::openScope() {
    scopes.emplace_back();
}

void SymbolTable::closeScope() {
    if (scopes.size() > 1) scopes.pop_back(); // el global nunca se cierra
}

bool SymbolTable::declare(const Symbol& symbol) {
    return scopes.back().emplace(symbol.name, symbol).second;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

const Symbol* SymbolTable::lookupCurrent(const std::string& name) const {
    auto found = scopes.back().find(name);
    return found == scopes.back().end() ? nullptr : &found->second;
}
