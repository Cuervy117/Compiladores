#include "ParseTree.hpp"

#include <cctype>
#include <sstream>

#include "Grammar.hpp"

bool Node::isEpsilon() const {
    return symbol == EPSILON;
}

bool Node::derivesEmpty() const {
    return children.size() == 1 && children[0]->isEpsilon();
}

namespace {

void collectLeaves(const Node* node, std::vector<const Node*>& out) {
    if (node->terminal) {
        if (!node->isEpsilon()) out.push_back(node);
        return;
    }
    for (const auto& c : node->children) collectLeaves(c.get(), out);
}

// En Mermaid las comillas, <, >, & y similares rompen la etiqueta,
// así que todo lo que no sea letra/número se manda como #código;
std::string escapeLabel(const std::string& text) {
    std::string out;
    for (unsigned char c : text) {
        if (std::isalnum(c) || c == ' ' || c == '_' || c == '.' || c == ',' || c == ':' || c >= 0x80) {
            out += static_cast<char>(c);
        } else {
            out += "#" + std::to_string(c) + ";";
        }
    }
    return out;
}

std::string escapeHtml(const std::string& text) {
    std::string out;
    for (char c : text) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            default:  out += c;
        }
    }
    return out;
}

class MermaidWriter {
public:
    explicit MermaidWriter(bool compact) : compact(compact) {}

    std::string write(const Node* root) {
        out << "flowchart TD\n";
        visit(root);
        out << "\n"
            << "    classDef nt fill:#e8eef9,stroke:#3b5b92,color:#14213d\n"
            << "    classDef tok fill:#fff4d6,stroke:#b7791f,color:#3d2c00\n"
            << "    classDef eps fill:#f4f4f4,stroke:#b0b0b0,color:#8a8a8a,stroke-dasharray:3 3\n"
            << "    classDef pending fill:#ffffff,stroke:#9e9e9e,color:#9e9e9e,stroke-dasharray:5 3\n"
            << "    classDef err fill:#fde2e1,stroke:#c0392b,color:#7b1d14,stroke-width:2px\n";
        return out.str();
    }

private:
    bool compact;
    int nextId = 0;
    std::ostringstream out;

    // Ramas que en modo compacto no se dibujan: ε y no terminales
    // ya expandidos que al final no generaron ningún token.
    bool hidden(const Node* n) const {
        if (!compact) return false;
        if (n->terminal) return n->isEpsilon();
        if (!n->done) return false;
        for (const auto& c : n->children) {
            if (!hidden(c.get())) return false;
        }
        return true;
    }

    std::vector<const Node*> visibleChildren(const Node* n) const {
        std::vector<const Node*> result;
        for (const auto& c : n->children) {
            if (!hidden(c.get())) result.push_back(c.get());
        }
        return result;
    }

    int visit(const Node* n) {
        const int id = nextId++;
        const Node* bottom = n;
        bool failed = n->failed;
        std::string label = n->symbol;

        if (compact && !n->terminal) {
            // Expr -> OrExpr -> ... -> Primary se dibuja como un solo nodo
            while (true) {
                std::vector<const Node*> kids = visibleChildren(bottom);
                if (kids.size() != 1 || kids[0]->terminal || !kids[0]->done) break;
                bottom = kids[0];
                failed = failed || bottom->failed;
            }
            if (bottom != n) label += " › " + bottom->symbol;
        }

        out << "    n" << id << shape(n, label) << ":::" << style(n, failed) << "\n";

        const std::vector<const Node*> kids = compact ? visibleChildren(bottom) : visibleChildren(n);
        for (const Node* child : kids) {
            const int childId = visit(child);
            out << "    n" << id << " --> n" << childId << "\n";
        }
        return id;
    }

    static std::string shape(const Node* n, const std::string& symbolLabel) {
        if (!n->terminal || n->isEpsilon()) {
            return "[\"" + escapeLabel(symbolLabel) + "\"]";
        }
        std::string text = n->symbol;
        // Para id y constantes se muestra también el lexema
        if (n->done && n->token.value != n->symbol) text += ": " + n->token.value;
        return "(\"" + escapeLabel(text) + "\")";
    }

    static const char* style(const Node* n, bool failed) {
        if (failed) return "err";
        if (n->isEpsilon()) return "eps";
        if (!n->done) return "pending";
        return n->terminal ? "tok" : "nt";
    }
};

}

std::vector<const Node*> leaves(const Node* node) {
    std::vector<const Node*> out;
    collectLeaves(node, out);
    return out;
}

std::string toMermaid(const Node* root, bool compact) {
    MermaidWriter writer(compact);
    return writer.write(root);
}

std::string mermaidPage(const std::string& diagram, const std::string& title) {
    std::ostringstream html;
    html << "<!doctype html>\n"
         << "<html lang=\"es\">\n"
         << "<head>\n"
         << "  <meta charset=\"utf-8\">\n"
         << "  <title>" << escapeHtml(title) << "</title>\n"
         << "  <style>\n"
         << "    body { font-family: system-ui, sans-serif; margin: 1.5rem; color: #222; }\n"
         << "    h1 { font-size: 1.2rem; margin: 0 0 .25rem; }\n"
         << "    p { margin: 0 0 1rem; color: #666; font-size: .9rem; }\n"
         << "    .tree { overflow: auto; border: 1px solid #ddd; border-radius: 6px; padding: 1rem; }\n"
         << "  </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "  <h1>" << escapeHtml(title) << "</h1>\n"
         << "  <p>Parse tree generado por el parser LL(1) &middot; UNAM FI Compiladores G5.03</p>\n"
         << "  <div class=\"tree\">\n"
         << "<pre class=\"mermaid\">\n" << escapeHtml(diagram) << "</pre>\n"
         << "  </div>\n"
         << "  <script type=\"module\">\n"
         << "    import mermaid from 'https://cdn.jsdelivr.net/npm/mermaid@11/dist/mermaid.esm.min.mjs';\n"
         << "    mermaid.initialize({\n"
         << "      startOnLoad: true,\n"
         << "      maxEdges: 5000,\n"
         << "      maxTextSize: 1000000,\n"
         << "      flowchart: { useMaxWidth: false, nodeSpacing: 20, rankSpacing: 40 }\n"
         << "    });\n"
         << "  </script>\n"
         << "</body>\n"
         << "</html>\n";
    return html.str();
}
