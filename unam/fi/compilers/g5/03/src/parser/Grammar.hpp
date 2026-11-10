#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

inline const std::string EPSILON = "ε";
inline const std::string END_MARK = "$";

// A -> X1 X2 ... Xn 
struct Production {
    int number;
    std::string lhs;
    std::vector<std::string> rhs;
};

// Dos producciones que caen en la misma celda M[A, a]
struct Conflict {
    std::string nonterminal;
    std::string terminal;
    int kept;
    int dropped;
};

// Gramática libre de contexto 

class Grammar {
public:
    explicit Grammar(const std::string& text);

    const std::string& start() const { return startSymbol; }
    const std::vector<Production>& productions() const { return prods; }
    const std::vector<std::string>& nonterminals() const { return nonterms; }
    const std::set<std::string>& terminals() const { return terms; }

    bool isNonterminal(const std::string& symbol) const;

    const std::set<std::string>& first(const std::string& symbol) const;
    const std::set<std::string>& follow(const std::string& nonterminal) const;
    std::set<std::string> firstOf(const std::vector<std::string>& sequence) const;

    // M[A, a]. nullptr si la celda está vacía (error de sintaxis)
    const Production* predict(const std::string& nonterminal, const std::string& terminal) const;

    const Production* emptyProduction(const std::string& nonterminal) const;

    // Terminales con entrada en la fila de A, para armar mensajes de error
    std::vector<std::string> expected(const std::string& nonterminal) const;

    // Conflictos que se resolvieron al construir la tabla
    const std::vector<Conflict>& conflicts() const { return resolved; }

    // Gramática numerada, FIRST, FOLLOW y tabla en texto
    std::string report() const;

private:
    struct Cell {
        int production;
        bool fromFollow; 
    };

    std::string startSymbol;
    std::vector<Production> prods;
    std::vector<std::string> nonterms;     // en el orden en que aparecen
    std::set<std::string> nontermSet;
    std::set<std::string> terms;

    std::map<std::string, std::set<std::string>> firstSets;
    std::map<std::string, std::set<std::string>> followSets;
    std::map<std::string, std::map<std::string, Cell>> table;
    std::vector<Conflict> resolved;

    void load(const std::string& text);
    void computeFirst();
    void computeFollow();
    void buildTable();
    void setCell(const std::string& a, const std::string& t, int production, bool fromFollow);
};

std::string productionText(const Production& p);
