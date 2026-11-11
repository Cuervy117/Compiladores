#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../Token.hpp"

// Nodo del árbol de derivación. Las hojas son terminales (o ε) y los nodos
// internos son no terminales con los hijos de la producción que se aplicó.
struct Node {
    std::string symbol;
    bool terminal = false;
    Token token;          // Solo en terminales ya emparejados
    int line = 0;
    bool done = false;    // Terminal emparejado / no terminal expandido
    bool failed = false;  // Aquí se detuvo el parser por un error
    std::vector<std::unique_ptr<Node>> children;

    Node(std::string sym, bool isTerminal) : symbol(std::move(sym)), terminal(isTerminal) {}

    bool isEpsilon() const;
    const Node* child(std::size_t i) const { return children.at(i).get(); }

    // A -> ε (su único hijo es la hoja ε)
    bool derivesEmpty() const;
};

// Hojas de izquierda a derecha sin contar ε (la "frontera" del árbol)
std::vector<const Node*> leaves(const Node* node);

// compact = quita las ramas que solo derivan ε y junta las cadenas
// de no terminales con un solo hijo (Expr › ... › Primary)
std::string toMermaid(const Node* root, bool compact);

// Página HTML que dibuja el diagrama con Mermaid (desde CDN)
std::string mermaidPage(const std::string& diagram, const std::string& title);
