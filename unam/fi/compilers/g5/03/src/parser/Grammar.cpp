#include "Grammar.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace {

std::string join(const std::set<std::string>& items) {
    std::string out;
    for (const std::string& s : items) {
        if (!out.empty()) out += ' ';
        out += s;
    }
    return out;
}

std::string pad(const std::string& s, std::size_t width) {
    return s + std::string(width > s.size() ? width - s.size() : 1, ' ');
}

}

std::string productionText(const Production& p) {
    std::string out = p.lhs + " ->";
    if (p.rhs.empty()) return out + " " + EPSILON;
    for (const std::string& s : p.rhs) out += " " + s;
    return out;
}

Grammar::Grammar(const std::string& text) {
    load(text);
    computeFirst();
    computeFollow();
    buildTable();
}

void Grammar::load(const std::string& text) {
    std::istringstream lines(text);
    std::string line;
    std::string current; // lado izquierdo de la regla que se está leyendo

    while (std::getline(lines, line)) {
        std::istringstream in(line);
        std::vector<std::string> words;
        std::string w;
        while (in >> w) words.push_back(w);
        if (words.empty() || words[0][0] == '#') continue;

        std::size_t i = 0;
        if (words.size() >= 2 && words[1] == "->") {
            current = words[0];
            if (startSymbol.empty()) startSymbol = current;
            if (nontermSet.insert(current).second) nonterms.push_back(current);
            i = 2;
        } else if (words[0] == "|" && !current.empty()) {
            i = 1; // alternativa en una línea nueva
        } else {
            throw std::runtime_error("Regla mal escrita en la gramatica: " + line);
        }

        std::vector<std::string> rhs;
        for (; i <= words.size(); ++i) {
            if (i == words.size() || words[i] == "|") {
                prods.push_back({static_cast<int>(prods.size()) + 1, current, rhs});
                rhs.clear();
            } else if (words[i] != EPSILON) {
                rhs.push_back(words[i]);
            }
        }
    }

    if (prods.empty()) throw std::runtime_error("La gramatica esta vacia");

    for (const Production& p : prods) {
        for (const std::string& s : p.rhs) {
            if (!isNonterminal(s)) terms.insert(s);
        }
    }
}

bool Grammar::isNonterminal(const std::string& symbol) const {
    return nontermSet.count(symbol) > 0;
}

const std::set<std::string>& Grammar::first(const std::string& symbol) const {
    static const std::set<std::string> empty;
    auto it = firstSets.find(symbol);
    return it == firstSets.end() ? empty : it->second;
}

const std::set<std::string>& Grammar::follow(const std::string& nonterminal) const {
    static const std::set<std::string> empty;
    auto it = followSets.find(nonterminal);
    return it == followSets.end() ? empty : it->second;
}

// FIRST(X1 X2 ... Xn)
std::set<std::string> Grammar::firstOf(const std::vector<std::string>& sequence) const {
    std::set<std::string> result;
    for (const std::string& symbol : sequence) {
        const std::set<std::string>& f = first(symbol);
        bool nullable = false;
        for (const std::string& t : f) {
            if (t == EPSILON) nullable = true;
            else result.insert(t);
        }
        if (!nullable) return result;
    }
    result.insert(EPSILON);
    return result;
}

void Grammar::computeFirst() {
    for (const std::string& t : terms) firstSets[t] = {t};
    for (const std::string& a : nonterms) firstSets[a];

    // Punto fijo: se repite hasta que ningún conjunto crezca
    bool changed = true;
    while (changed) {
        changed = false;
        for (const Production& p : prods) {
            std::set<std::string>& target = firstSets[p.lhs];
            const std::size_t before = target.size();
            std::set<std::string> f = firstOf(p.rhs);
            target.insert(f.begin(), f.end());
            if (target.size() != before) changed = true;
        }
    }
}

void Grammar::computeFollow() {
    for (const std::string& a : nonterms) followSets[a];
    followSets[startSymbol].insert(END_MARK);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const Production& p : prods) {
            for (std::size_t i = 0; i < p.rhs.size(); ++i) {
                const std::string& b = p.rhs[i];
                if (!isNonterminal(b)) continue;

                std::vector<std::string> beta(p.rhs.begin() + i + 1, p.rhs.end());
                std::set<std::string> f = firstOf(beta);

                std::set<std::string>& target = followSets[b];
                const std::size_t before = target.size();
                for (const std::string& t : f) {
                    if (t != EPSILON) target.insert(t);
                }
                if (f.count(EPSILON)) {
                    const std::set<std::string>& fa = followSets[p.lhs];
                    target.insert(fa.begin(), fa.end());
                }
                if (target.size() != before) changed = true;
            }
        }
    }
}

void Grammar::buildTable() {
    for (const Production& p : prods) {
        std::set<std::string> f = firstOf(p.rhs);
        for (const std::string& a : f) {
            if (a != EPSILON) setCell(p.lhs, a, p.number, false);
        }
        if (f.count(EPSILON)) {
            for (const std::string& b : follow(p.lhs)) {
                setCell(p.lhs, b, p.number, true);
            }
        }
    }
}

void Grammar::setCell(const std::string& a, const std::string& t, int production, bool fromFollow) {
    auto& row = table[a];
    auto it = row.find(t);
    if (it == row.end()) {
        row[t] = {production, fromFollow};
        return;
    }
    if (it->second.production == production) return;

    Cell& old = it->second;
    if (old.fromFollow != fromFollow) {
        const Cell winner = old.fromFollow ? Cell{production, fromFollow} : old;
        const int loser = old.fromFollow ? old.production : production;
        resolved.push_back({a, t, winner.production, loser});
        old = winner;
        return;
    }

    throw std::runtime_error("La gramatica no es LL(1): conflicto en M[" + a + ", " + t + "] entre " +
                             productionText(prods[old.production - 1]) + " y " +
                             productionText(prods[production - 1]));
}

const Production* Grammar::predict(const std::string& nonterminal, const std::string& terminal) const {
    auto row = table.find(nonterminal);
    if (row == table.end()) return nullptr;
    auto cell = row->second.find(terminal);
    if (cell == row->second.end()) return nullptr;
    return &prods[cell->second.production - 1];
}

const Production* Grammar::emptyProduction(const std::string& nonterminal) const {
    for (const Production& p : prods) {
        if (p.lhs == nonterminal && firstOf(p.rhs).count(EPSILON)) return &p;
    }
    return nullptr;
}

std::vector<std::string> Grammar::expected(const std::string& nonterminal) const {
    std::vector<std::string> out;
    auto row = table.find(nonterminal);
    if (row == table.end()) return out;
    for (const auto& entry : row->second) out.push_back(entry.first);
    return out;
}

std::string Grammar::report() const {
    std::size_t width = 0;
    for (const std::string& a : nonterms) width = std::max(width, a.size());
    width += 2;

    std::ostringstream out;
    out << "GRAMMAR (" << prods.size() << " productions, start symbol " << startSymbol << ")\n\n";
    for (const Production& p : prods) {
        out << "  (" << p.number << ")" << (p.number < 10 ? "  " : " ") << productionText(p) << "\n";
    }

    out << "\nTERMINALS\n  " << join(terms) << " " << END_MARK << "\n";

    out << "\nFIRST / FOLLOW\n\n";
    for (const std::string& a : nonterms) {
        out << "  " << pad(a, width) << "FIRST  = { " << join(first(a)) << " }\n";
        out << "  " << pad("", width) << "FOLLOW = { " << join(follow(a)) << " }\n";
    }

    out << "\nLL(1) TABLE  M[A, a] (empty cells are syntax errors)\n\n";
    for (const std::string& a : nonterms) {
        // Agrupamos los terminales por producción para que quepa en pantalla
        std::map<int, std::set<std::string>> byProduction;
        auto row = table.find(a);
        if (row == table.end()) continue;
        for (const auto& entry : row->second) byProduction[entry.second.production].insert(entry.first);

        for (const auto& group : byProduction) {
            out << "  " << pad(a, width) << "(" << group.first << ") on { " << join(group.second) << " }\n";
        }
    }

    out << "\nCONFLICTS\n";
    if (resolved.empty()) out << "  none\n";
    for (const Conflict& c : resolved) {
        out << "  M[" << c.nonterminal << ", " << c.terminal << "] had (" << c.kept << ") and ("
            << c.dropped << "); kept " << productionText(prods[c.kept - 1]) << "\n";
    }
    return out.str();
}
