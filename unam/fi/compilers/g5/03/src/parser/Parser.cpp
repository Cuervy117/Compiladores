#include "Parser.hpp"

namespace {

const std::string NO_TERMINAL = "?"; // token que no corresponde a ningún terminal

bool closedLiteral(const std::string& v, char quote) {
    if (v.size() < 2 || v.back() != quote) return false;
    // La comilla final no debe estar escapada: "abc\"
    std::size_t slashes = 0;
    for (std::size_t i = v.size() - 2; i > 0 && v[i] == '\\'; --i) ++slashes;
    return slashes % 2 == 0;
}

std::string constantTerminal(const std::string& v) {
    if (v[0] == '"') return closedLiteral(v, '"') ? "string_lit" : NO_TERMINAL;
    if (v[0] == '\'') return closedLiteral(v, '\'') ? "char_const" : NO_TERMINAL;
    if (v.find_first_of(".eE") != std::string::npos) return "float_const";
    return "int_const";
}

std::string describe(const Token& t) {
    if (t.type == TokenType::EndOfFile) return "end of input";
    return "'" + t.value + "'";
}

// Errores que en realidad vienen del lexer
bool lexicalError(const Token& t, std::string& message) {
    if (t.type == TokenType::Unknown) {
        message = "lexical error, unrecognized character '" + t.value + "'";
        return true;
    }
    if (t.type == TokenType::Constant && constantTerminal(t.value) == NO_TERMINAL) {
        message = "lexical error, unterminated literal " + t.value;
        return true;
    }
    return false;
}

std::string stackText(const std::vector<Node*>& stack) {
    std::string out;
    for (const Node* n : stack) {
        if (!out.empty()) out += ' ';
        out += n->symbol;
    }
    return out;
}

std::string inputText(const std::vector<Token>& input, std::size_t from) {
    std::string out;
    for (std::size_t i = from; i < input.size(); ++i) {
        if (!out.empty()) out += ' ';
        out += terminalFor(input[i]);
    }
    return out;
}

}

std::string terminalFor(const Token& token) {
    switch (token.type) {
        case TokenType::Identifier: return "id";
        case TokenType::Constant:   return constantTerminal(token.value);
        case TokenType::Keyword:
        case TokenType::Operator:
        case TokenType::Punctuation: return token.value;
        case TokenType::EndOfFile:  return END_MARK;
        default:                    return NO_TERMINAL;
    }
}

ParseResult Parser::parse(const std::vector<Token>& tokens, bool withTrace) const {
    ParseResult result;

    std::vector<Token> input(tokens);
    Token end{TokenType::EndOfFile, END_MARK};
    end.line = tokens.empty() ? 1 : tokens.back().line;
    input.push_back(end);

    // Pila inicial: $ S  (el tope es el final del vector)
    result.tree = std::make_unique<Node>(grammar.start(), false);
    Node bottom(END_MARK, true);
    std::vector<Node*> stack = {&bottom, result.tree.get()};
    std::size_t pos = 0;

    auto fail = [&](Node* at, int line, const std::string& message) {
        at->failed = true;
        result.errorLine = line;
        result.errorMessage = message;
        if (withTrace) result.trace.back().action = "ERROR";
    };

    auto unexpectedIn = [&](const std::string& nonterminal, const Token& look) {
        std::string options;
        const std::vector<std::string> exp = grammar.expected(nonterminal);
        for (std::size_t i = 0; i < exp.size() && i < 12; ++i) {
            options += (i == 0 ? "'" : ", '") + exp[i] + "'";
        }
        if (exp.size() > 12) options += ", ...";
        return "unexpected " + describe(look) + " in " + nonterminal + ", expected one of: " + options +
               " (M[" + nonterminal + ", " + terminalFor(look) + "] is empty)";
    };

    // Primer no terminal que tomó su producción ε por defecto en la posición
    // actual. Si después el error sale contra el $ del fondo, el problema
    // real estaba en ese no terminal.
    std::string defaulted;
    std::size_t defaultedPos = input.size();

    while (true) {
        Node* top = stack.back();
        const Token& look = input[pos];
        const std::string a = terminalFor(look);

        if (withTrace) result.trace.push_back({stackText(stack), inputText(input, pos), ""});

        std::string lexMessage;
        if (top->terminal) {
            if (top->symbol == a) {
                if (a == END_MARK) {
                    if (withTrace) result.trace.back().action = "accept";
                    result.ok = true;
                    return result;
                }
                top->token = look;
                top->line = look.line;
                top->done = true;
                stack.pop_back();
                ++pos;
                if (withTrace) result.trace.back().action = "match " + a;
                continue;
            }

            // Si faltó un ';' o ')' lo natural es reportar la línea del token
            // anterior, que es donde debió estar (igual que gcc).
            const int line = pos > 0 ? input[pos - 1].line : look.line;
            if (lexicalError(look, lexMessage)) {
                fail(top, look.line, lexMessage);
            } else if (top == &bottom && defaultedPos == pos) {
                fail(top, look.line, unexpectedIn(defaulted, look));
            } else if (top == &bottom) {
                fail(top, look.line, "unexpected " + describe(look) + " after the end of the program");
            } else {
                fail(top, line, "expected '" + top->symbol + "' before " + describe(look));
            }
            return result;
        }

        const Production* p = grammar.predict(top->symbol, a);
        bool byDefault = false;
        if (p == nullptr) {
            p = grammar.emptyProduction(top->symbol);
            byDefault = true;
            if (p != nullptr && defaultedPos != pos) {
                defaulted = top->symbol;
                defaultedPos = pos;
            }
        }

        if (p == nullptr) {
            if (lexicalError(look, lexMessage)) {
                fail(top, look.line, lexMessage);
                return result;
            }
            fail(top, look.line, unexpectedIn(top->symbol, look));
            return result;
        }

        // Expandir A -> X1 ... Xn: los hijos se agregan en orden y se
        // apilan al revés para que X1 quede en el tope.
        stack.pop_back();
        top->done = true;
        top->line = look.line;
        if (withTrace) {
            result.trace.back().action = productionText(*p) + (byDefault ? "  (default)" : "");
        }

        if (p->rhs.empty()) {
            auto eps = std::make_unique<Node>(EPSILON, true);
            eps->done = true;
            eps->line = look.line;
            top->children.push_back(std::move(eps));
            continue;
        }
        for (const std::string& symbol : p->rhs) {
            top->children.push_back(std::make_unique<Node>(symbol, !grammar.isNonterminal(symbol)));
        }
        for (auto it = top->children.rbegin(); it != top->children.rend(); ++it) {
            stack.push_back(it->get());
        }
    }
}
