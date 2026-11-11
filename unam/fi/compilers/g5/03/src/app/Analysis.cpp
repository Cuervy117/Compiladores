#include "Analysis.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include "../Lexer.hpp"
#include "../parser/CGrammar.hpp"
#include "../parser/Parser.hpp"
#include "../semantic/Semantic.hpp"

namespace {

// Más pasos que esto no se ven bien en la página y el JSON crece mucho
const std::size_t MAX_TRACE = 3000;

std::string quote(const std::string& s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    const char* hex = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[c >> 4];
                    out += hex[c & 15];
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out + "\"";
}

std::string stringList(const std::vector<std::string>& items) {
    std::string out = "[";
    for (std::size_t i = 0; i < items.size(); ++i) out += (i ? "," : "") + quote(items[i]);
    return out + "]";
}

std::string diagnostics(const std::vector<Diagnostic>& list) {
    std::string out = "[";
    for (std::size_t i = 0; i < list.size(); ++i) {
        out += (i ? "," : "");
        out += "{\"line\":" + std::to_string(list[i].line) + ",\"message\":" + quote(list[i].message) + "}";
    }
    return out + "]";
}

std::string categoryOf(const Token& t) {
    return t.type == TokenType::Unknown ? "unknown" : getCategoryName(t.type);
}

} 

std::string analyzeToJson(const std::string& source) {
    std::ostringstream json;
    const std::vector<Token> tokens = scan(source);

    // Tokens y conteo por categoría (igual que la tabla del lexer)
    std::map<std::string, int> counts = {
        {"keyword", 0}, {"identifier", 0}, {"operator", 0}, {"constant", 0}, {"punctuation", 0}, {"unknown", 0}};
    json << "{\"tokens\":[";
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const Token& t = tokens[i];
        const std::string category = categoryOf(t);
        ++counts[category];
        json << (i ? "," : "") << "{\"n\":" << i + 1 << ",\"lexeme\":" << quote(t.value)
             << ",\"category\":" << quote(category) << ",\"line\":" << t.line
             << ",\"terminal\":" << quote(terminalFor(t)) << "}";
    }
    json << "],\"total\":" << tokens.size() << ",\"counts\":{";
    const char* order[] = {"keyword", "identifier", "operator", "constant", "punctuation", "unknown"};
    for (int i = 0; i < 6; ++i) json << (i ? "," : "") << quote(order[i]) << ":" << counts[order[i]];
    json << "}";

    // Parser
    const Parser parser(cGrammar());
    const ParseResult result = parser.parse(tokens, true);
    json << ",\"parse\":{\"ok\":" << (result.ok ? "true" : "false")
         << ",\"line\":" << result.errorLine << ",\"message\":" << quote(result.errorMessage) << "}";

    // SDT (solo si el árbol está completo)
    Semantic sdt;
    const bool evaluated = result.ok;
    const bool verified = evaluated && sdt.check(result.tree.get());
    json << ",\"sdt\":{\"evaluated\":" << (evaluated ? "true" : "false")
         << ",\"ok\":" << (verified ? "true" : "false")
         << ",\"errors\":" << diagnostics(sdt.errors())
         << ",\"warnings\":" << diagnostics(sdt.warnings()) << "}";

    json << ",\"symbols\":[";
    const std::vector<SymbolRecord>& log = sdt.symbolLog();
    for (std::size_t i = 0; i < log.size(); ++i) {
        const SymbolRecord& s = log[i];
        json << (i ? "," : "") << "{\"name\":" << quote(s.name) << ",\"kind\":" << quote(s.kind)
             << ",\"type\":" << quote(s.type) << ",\"params\":" << quote(s.params)
             << ",\"line\":" << s.line << ",\"scope\":" << quote(s.scope) << ",\"depth\":" << s.depth << "}";
    }
    json << "]";

    // Lo mismo que imprime el CLI en sus primeras líneas
    std::vector<std::string> summary;
    summary.push_back(result.ok ? "Parsing Success!" : "Parsing error...");
    summary.push_back(verified ? "SDT Verified!" : "SDT error...");
    json << ",\"summary\":" << stringList(summary);

    json << ",\"tree\":{\"full\":" << quote(toMermaid(result.tree.get(), false))
         << ",\"compact\":" << quote(toMermaid(result.tree.get(), true)) << "}";

    json << ",\"trace\":[";
    const std::size_t steps = std::min(result.trace.size(), MAX_TRACE);
    for (std::size_t i = 0; i < steps; ++i) {
        const TraceStep& s = result.trace[i];
        json << (i ? "," : "") << "[" << quote(s.stack) << "," << quote(s.input) << "," << quote(s.action) << "]";
    }
    json << "],\"traceTruncated\":" << (result.trace.size() > MAX_TRACE ? "true" : "false") << "}";
    return json.str();
}

std::string grammarToJson() {
    const Grammar& g = cGrammar();
    std::ostringstream json;

    json << "{\"start\":" << quote(g.start()) << ",\"productions\":[";
    const std::vector<Production>& prods = g.productions();
    for (std::size_t i = 0; i < prods.size(); ++i) {
        json << (i ? "," : "") << "{\"n\":" << prods[i].number << ",\"lhs\":" << quote(prods[i].lhs)
             << ",\"rhs\":" << stringList(prods[i].rhs) << "}";
    }

    std::vector<std::string> terminals(g.terminals().begin(), g.terminals().end());
    terminals.push_back(END_MARK);
    json << "],\"nonterminals\":" << stringList(g.nonterminals()) << ",\"terminals\":" << stringList(terminals);

    json << ",\"first\":{";
    for (std::size_t i = 0; i < g.nonterminals().size(); ++i) {
        const std::string& a = g.nonterminals()[i];
        const std::set<std::string>& f = g.first(a);
        json << (i ? "," : "") << quote(a) << ":" << stringList(std::vector<std::string>(f.begin(), f.end()));
    }
    json << "},\"follow\":{";
    for (std::size_t i = 0; i < g.nonterminals().size(); ++i) {
        const std::string& a = g.nonterminals()[i];
        const std::set<std::string>& f = g.follow(a);
        json << (i ? "," : "") << quote(a) << ":" << stringList(std::vector<std::string>(f.begin(), f.end()));
    }

    // Tabla como {A: {a: número de producción}}, solo celdas con entrada
    json << "},\"table\":{";
    for (std::size_t i = 0; i < g.nonterminals().size(); ++i) {
        const std::string& a = g.nonterminals()[i];
        json << (i ? "," : "") << quote(a) << ":{";
        bool first = true;
        for (const std::string& t : terminals) {
            const Production* p = g.predict(a, t);
            if (p == nullptr) continue;
            json << (first ? "" : ",") << quote(t) << ":" << p->number;
            first = false;
        }
        json << "}";
    }

    json << "},\"conflicts\":[";
    for (std::size_t i = 0; i < g.conflicts().size(); ++i) {
        const Conflict& c = g.conflicts()[i];
        json << (i ? "," : "") << "{\"nonterminal\":" << quote(c.nonterminal) << ",\"terminal\":" << quote(c.terminal)
             << ",\"kept\":" << c.kept << ",\"dropped\":" << c.dropped << "}";
    }
    json << "]}";
    return json.str();
}
