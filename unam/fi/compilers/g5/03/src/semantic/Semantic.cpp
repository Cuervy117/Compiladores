#include "Semantic.hpp"

#include <cctype>
#include <cstring>

namespace {

const Type ERROR_TYPE = makeType(BaseType::Error);
const Type INT_TYPE = makeType(BaseType::Int);

// Lexema de un terminal, o del único terminal de reglas como
// Type -> int, AddOp -> +, IoFunc -> printf
std::string textOf(const Node* n) {
    return n->terminal ? n->token.value : n->child(0)->token.value;
}

// Si toda la expresión es un solo token de cierto tipo, lo devuelve.
// Sirve para saber si algo es una variable (lado izquierdo de =, operando de &)
// o si el primer argumento de printf es un literal.
const Node* onlyLeaf(const Node* n, const std::string& terminal) {
    std::vector<const Node*> ls = leaves(n);
    return (ls.size() == 1 && ls[0]->symbol == terminal) ? ls[0] : nullptr;
}

// ArgList -> Expr ArgMore | ε      ArgMore -> , Expr ArgMore | ε
std::vector<const Node*> arguments(const Node* argList) {
    std::vector<const Node*> args;
    if (argList->derivesEmpty()) return args;
    args.push_back(argList->child(0));
    const Node* more = argList->child(1);
    while (!more->derivesEmpty()) {
        args.push_back(more->child(1));
        more = more->child(2);
    }
    return args;
}

Type promoted(Type t) {
    return t.base == BaseType::Char ? INT_TYPE : t;
}

struct FormatSpec {
    std::string text; // como aparece: %d, %5.2f, %lf
    char conversion;
    bool isLong;
};

// Saca los especificadores de formato de un literal "....".
// Es lo que en la entrega del lexer describía la regla F -> %d | %s | ...
std::vector<FormatSpec> formatSpecs(const std::string& literal, std::vector<std::string>& unknown) {
    std::vector<FormatSpec> specs;
    const std::string s = literal.substr(1, literal.size() - 2);

    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\') { ++i; continue; }
        if (s[i] != '%') continue;

        std::size_t j = i + 1;
        if (j < s.size() && s[j] == '%') { i = j; continue; } // %% imprime un %

        while (j < s.size() && std::strchr("-+ #0", s[j])) ++j;
        while (j < s.size() && std::isdigit(static_cast<unsigned char>(s[j]))) ++j;
        if (j < s.size() && s[j] == '.') {
            ++j;
            while (j < s.size() && std::isdigit(static_cast<unsigned char>(s[j]))) ++j;
        }
        bool isLong = false;
        if (j < s.size() && s[j] == 'l') { isLong = true; ++j; }

        if (j >= s.size()) {
            unknown.push_back(s.substr(i));
            break;
        }
        const std::string text = s.substr(i, j - i + 1);
        if (std::strchr("diuxXfFeEgGcs", s[j]) == nullptr) {
            unknown.push_back(text);
        } else {
            specs.push_back({text, s[j], isLong});
        }
        i = j;
    }
    return specs;
}

}

void Semantic::enterScope(const std::string& name) {
    symbols.openScope();
    scopeNames.push_back(name);
}

void Semantic::leaveScope() {
    symbols.closeScope();
    scopeNames.pop_back();
}

void Semantic::record(const Symbol& s, const std::string& kind) {
    SymbolRecord r;
    r.name = s.name;
    r.kind = kind;
    r.type = typeName(s.type);
    for (std::size_t i = 0; i < s.params.size(); ++i) {
        r.params += (i ? ", " : "") + typeName(s.params[i]);
    }
    r.line = s.line;
    r.scope = scopeNames.back();
    r.depth = static_cast<int>(scopeNames.size()) - 1;
    records.push_back(r);
}

void Semantic::error(int line, const std::string& message) {
    errorList.push_back({line, message});
}

void Semantic::warning(int line, const std::string& message) {
    warningList.push_back({line, message});
}

bool Semantic::check(const Node* root) {
    symbols = SymbolTable();
    errorList.clear();
    warningList.clear();
    records.clear();
    scopeNames = {"global"};
    insideFunction = false;
    loopDepth = 0;

    // Program -> ItemList      ItemList -> Item ItemList | ε
    const Node* list = root->child(0);
    while (!list->derivesEmpty()) {
        item(list->child(0));
        list = list->child(1);
    }
    return errorList.empty();
}

// Declaraciones

// Item -> Type id ItemTail | Stmt
// ItemTail -> ( ParamList ) Block | VarTail
void Semantic::item(const Node* n) {
    if (n->child(0)->symbol == "Stmt") {
        statement(n->child(0));
        return;
    }
    const Type type = typeFromKeyword(textOf(n->child(0)));
    const Node* id = n->child(1);
    const Node* tail = n->child(2);

    if (tail->child(0)->symbol == "(") {
        function(type, id, tail);
    } else {
        declarators(type, id, tail->child(0));
    }
}

void Semantic::function(Type type, const Node* id, const Node* tail) {
    const std::string& name = id->token.value;

    // ParamList -> Param ParamMore | ε     Param -> Type id
    std::vector<std::pair<Type, const Node*>> params;
    const Node* list = tail->child(1);
    if (!list->derivesEmpty()) {
        const Node* param = list->child(0);
        const Node* more = list->child(1);
        while (true) {
            params.push_back({typeFromKeyword(textOf(param->child(0))), param->child(1)});
            if (more->derivesEmpty()) break;
            param = more->child(1);
            more = more->child(2);
        }
    }

    Symbol fn;
    fn.name = name;
    fn.type = type;
    fn.isFunction = true;
    fn.line = id->line;
    for (const auto& p : params) fn.params.push_back(p.first);

    // Se declara antes del cuerpo para permitir recursión
    if (const Symbol* old = symbols.lookupCurrent(name)) {
        error(id->line, "redeclaration of '" + name + "' (first declared in line " + std::to_string(old->line) + ")");
    } else {
        symbols.declare(fn);
        record(fn, "function");
    }

    // Parámetros y cuerpo comparten ámbito, como en C
    enterScope(name + "()");
    for (const auto& p : params) {
        Symbol s;
        s.name = p.second->token.value;
        s.type = p.first;
        s.line = p.second->line;
        if (p.first.base == BaseType::Void) {
            error(s.line, "parameter '" + s.name + "' declared void");
            s.type = ERROR_TYPE;
        }
        if (symbols.declare(s)) record(s, "parameter");
        else error(s.line, "duplicate parameter '" + s.name + "'");
    }

    insideFunction = true;
    returnType = type;
    const Node* body = tail->child(3); // Block -> { BlockItems }
    blockItems(body->child(1));
    insideFunction = false;

    leaveScope();
}

// VarTail -> Init VarMore ;      VarMore -> , id Init VarMore | ε
// El tipo se hereda a cada declarador de la lista (int a = 1, b, c = 2;)
void Semantic::declarators(Type type, const Node* id, const Node* varTail) {
    variable(type, id, varTail->child(0));
    const Node* more = varTail->child(1);
    while (!more->derivesEmpty()) {
        variable(type, more->child(1), more->child(2));
        more = more->child(3);
    }
}

// Init -> = Expr | ε
void Semantic::variable(Type type, const Node* id, const Node* init) {
    const std::string& name = id->token.value;

    // Primero el inicializador y después se agrega a la tabla
    if (!init->derivesEmpty()) {
        Type value = expression(init->child(1));
        if (type.base != BaseType::Void) {
            checkAssign(type, value, id->line, "initialization of '" + name + "'");
        }
    }

    Symbol s;
    s.name = name;
    s.type = type;
    s.line = id->line;
    if (type.base == BaseType::Void) {
        error(id->line, "variable '" + name + "' declared void");
        s.type = ERROR_TYPE;
    }

    if (const Symbol* old = symbols.lookupCurrent(name)) {
        error(id->line, "redeclaration of '" + name + "' (first declared in line " + std::to_string(old->line) + ")");
        return;
    }
    symbols.declare(s);
    record(s, "variable");
}

// Sentencias

// BlockItems -> BlockItem BlockItems | ε
// BlockItem  -> Type id VarTail | Stmt
void Semantic::blockItems(const Node* n) {
    while (!n->derivesEmpty()) {
        const Node* bi = n->child(0);
        if (bi->child(0)->symbol == "Type") {
            declarators(typeFromKeyword(textOf(bi->child(0))), bi->child(1), bi->child(2));
        } else {
            statement(bi->child(0));
        }
        n = n->child(1);
    }
}

// Block -> { BlockItems }   (cada bloque abre un ámbito nuevo)
void Semantic::block(const Node* n) {
    enterScope("block (line " + std::to_string(n->line) + ")");
    blockItems(n->child(1));
    leaveScope();
}

void Semantic::loopBody(const Node* stmt) {
    ++loopDepth;
    statement(stmt);
    --loopDepth;
}

void Semantic::statement(const Node* n) {
    const Node* s = n->child(0);
    const std::string& kind = s->symbol;

    if (kind == "ExprStmt") {
        // ExprStmt -> Expr ; | ;
        if (s->child(0)->symbol == "Expr") expression(s->child(0));
    } else if (kind == "Block") {
        block(s);
    } else if (kind == "IfStmt") {
        // IfStmt -> if ( Expr ) Stmt ElsePart      ElsePart -> else Stmt | ε
        condition(s->child(2), "if");
        statement(s->child(4));
        const Node* elsePart = s->child(5);
        if (!elsePart->derivesEmpty()) statement(elsePart->child(1));
    } else if (kind == "WhileStmt") {
        // WhileStmt -> while ( Expr ) Stmt
        condition(s->child(2), "while");
        loopBody(s->child(4));
    } else if (kind == "DoStmt") {
        // DoStmt -> do Stmt while ( Expr ) ;
        loopBody(s->child(1));
        condition(s->child(4), "do-while");
    } else if (kind == "ForStmt") {
        forStatement(s);
    } else if (kind == "ReturnStmt") {
        returnStatement(s);
    } else if (kind == "IoStmt") {
        ioStatement(s);
    } else if (kind == "break" || kind == "continue") {
        if (loopDepth == 0) error(s->line, "'" + kind + "' outside of a loop");
    }
}

// ForStmt -> for ( ForInit OptExpr ; OptExpr ) Stmt
// ForInit -> Type id VarTail | ExprStmt
void Semantic::forStatement(const Node* n) {
    enterScope("for (line " + std::to_string(n->line) + ")"); // la variable de control solo vive dentro del for

    const Node* init = n->child(2);
    if (init->child(0)->symbol == "Type") {
        declarators(typeFromKeyword(textOf(init->child(0))), init->child(1), init->child(2));
    } else if (init->child(0)->child(0)->symbol == "Expr") {
        expression(init->child(0)->child(0));
    }

    const Node* cond = n->child(3);
    if (!cond->derivesEmpty()) condition(cond->child(0), "for");
    const Node* step = n->child(5);
    if (!step->derivesEmpty()) expression(step->child(0));

    loopBody(n->child(7));
    leaveScope();
}

// ReturnStmt -> return OptExpr ;
void Semantic::returnStatement(const Node* n) {
    const Node* opt = n->child(1);
    const bool hasValue = !opt->derivesEmpty();
    const Type value = hasValue ? expression(opt->child(0)) : makeType(BaseType::Void);

    if (!insideFunction) {
        error(n->line, "'return' outside of a function");
    } else if (!hasValue && returnType.base != BaseType::Void) {
        error(n->line, "'return' without a value in a function returning " + typeName(returnType));
    } else if (hasValue && returnType.base == BaseType::Void) {
        error(n->line, "'return' with a value in a void function");
    } else if (hasValue) {
        checkAssign(returnType, value, n->line, "return value");
    }
}

// IoStmt -> IoFunc ( ArgList ) ;
void Semantic::ioStatement(const Node* n) {
    const std::string name = textOf(n->child(0));
    const std::vector<const Node*> args = arguments(n->child(2));

    if (name == "print") {
        // print(...) es nuestro: acepta uno o más valores de cualquier tipo
        if (args.empty()) error(n->line, "print needs at least one argument");
        for (std::size_t i = 0; i < args.size(); ++i) {
            Type t = expression(args[i]);
            if (!t.pointer && t.base == BaseType::Void) {
                error(n->line, "argument " + std::to_string(i + 1) + " of print has type void");
            }
        }
        return;
    }
    formatCall(name, args, n->line);
}

// printf y scanf: el formato debe ser un literal y cada especificador
// tiene que coincidir en cantidad y tipo con los argumentos que siguen.
void Semantic::formatCall(const std::string& name, const std::vector<const Node*>& args, int line) {
    if (args.empty()) {
        error(line, name + " needs a format string");
        return;
    }

    const Node* format = onlyLeaf(args[0], "string_lit");
    if (format == nullptr) expression(args[0]);

    std::vector<Type> types;
    for (std::size_t i = 1; i < args.size(); ++i) types.push_back(expression(args[i]));

    if (format == nullptr) {
        error(line, "the first argument of " + name + " must be a string literal");
        return;
    }

    std::vector<std::string> unknown;
    const std::vector<FormatSpec> specs = formatSpecs(format->token.value, unknown);
    for (const std::string& u : unknown) {
        error(line, "unknown format specifier '" + u + "' in " + name);
    }

    if (specs.size() != types.size()) {
        error(line, name + " format has " + std::to_string(specs.size()) + " specifier(s) but " +
                    std::to_string(types.size()) + " argument(s) were given");
    }

    const bool reading = (name == "scanf");
    for (std::size_t i = 0; i < specs.size() && i < types.size(); ++i) {
        const FormatSpec& f = specs[i];
        const Type t = types[i];
        const std::string where = "argument " + std::to_string(i + 2) + " of " + name;
        if (isError(t)) continue;

        bool ok = false;
        std::string want;
        if (reading) {
            if (!t.pointer) {
                error(line, where + " must be an address (&variable) for '" + f.text + "'");
                continue;
            }
            BaseType base = BaseType::Char;
            if (std::strchr("diuxX", f.conversion)) base = BaseType::Int;
            else if (std::strchr("fFeEgG", f.conversion)) base = f.isLong ? BaseType::Double : BaseType::Float;
            ok = (t.base == base);
            want = typeName(makeType(base, true));
        } else if (std::strchr("diuxXc", f.conversion)) {
            ok = isIntegral(t);
            want = (f.conversion == 'c') ? "char" : "int";
        } else if (std::strchr("fFeEgG", f.conversion)) {
            ok = isFloating(t);
            want = "double";
        } else { // %s
            ok = t.pointer && t.base == BaseType::Char;
            want = "char*";
        }

        if (!ok) {
            error(line, "'" + f.text + "' expects " + want + " but " + where + " has type " + typeName(t));
        }
    }
}

void Semantic::condition(const Node* expr, const std::string& where) {
    const Type t = expression(expr);
    if (!isError(t) && !isScalar(t)) {
        error(expr->line, "the condition of '" + where + "' must be a scalar value, not " + typeName(t));
    }
}

// Expresiones

// Expr -> OrExpr AssignTail       AssignTail -> AssignOp Expr | ε
// La asignación es recursiva por la derecha (a = b = c). El parser acepta
// cualquier OrExpr a la izquierda; que sea una variable se revisa aquí.
Type Semantic::expression(const Node* n) {
    const Node* left = n->child(0);
    const Node* tail = n->child(1);

    Type target = binary(left);
    if (tail->derivesEmpty()) return target;

    const std::string op = textOf(tail->child(0));
    Type value = expression(tail->child(1));

    const Node* var = onlyLeaf(left, "id");
    if (var == nullptr) {
        error(tail->line, "left side of '" + op + "' is not a variable");
        return ERROR_TYPE;
    }
    if (isError(target)) return target;

    if (op != "=") {
        // a += b  se revisa como  a = a + b
        value = operation(op.substr(0, op.size() - 1), target, value, tail->line);
        if (isError(value)) return value;
    }
    checkAssign(target, value, tail->line, "assignment to '" + var->token.value + "'");
    return target;
}

// OrExpr, AndExpr, EqExpr, RelExpr, AddExpr y MulExpr tienen la misma forma:
//   X     -> Y XTail            XTail.inh = Y.type ;  X.type = XTail.syn
//   XTail -> op Y XTail1        XTail1.inh = op(XTail.inh, Y.type)
//   XTail -> ε                  XTail.syn = XTail.inh
// Por eso un solo par de funciones sirve para todos los niveles.
Type Semantic::binary(const Node* n) {
    if (n->symbol == "Unary") return unary(n);
    const Type left = binary(n->child(0));
    return binaryTail(n->child(1), left);
}

Type Semantic::binaryTail(const Node* tail, Type inherited) {
    if (tail->derivesEmpty()) return inherited;
    const std::string op = textOf(tail->child(0));
    const Type right = binary(tail->child(1));
    return binaryTail(tail->child(2), operation(op, inherited, right, tail->line));
}

Type Semantic::operation(const std::string& op, Type a, Type b, int line) {
    if (isError(a) || isError(b)) return ERROR_TYPE;

    if (op == "||" || op == "&&") {
        if (isScalar(a) && isScalar(b)) return INT_TYPE;
    } else if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
        if ((isNumeric(a) && isNumeric(b)) || (a.pointer && sameType(a, b))) return INT_TYPE;
    } else if (op == "%") {
        if (isIntegral(a) && isIntegral(b)) return INT_TYPE;
        error(line, "operator '%' needs integer operands, got " + typeName(a) + " and " + typeName(b));
        return ERROR_TYPE;
    } else if (isNumeric(a) && isNumeric(b)) {
        // + - * /  el resultado es el tipo "más grande" (char sube a int)
        return promoted(rank(a) >= rank(b) ? a : b);
    }

    error(line, "invalid operands to '" + op + "' (" + typeName(a) + " and " + typeName(b) + ")");
    return ERROR_TYPE;
}

// Unary -> UnaryOp Unary | Primary
Type Semantic::unary(const Node* n) {
    if (n->child(0)->symbol == "Primary") return primary(n->child(0));

    const std::string op = textOf(n->child(0));
    const Node* operand = n->child(1);
    const Type t = unary(operand);
    if (isError(t)) return t;

    if (op == "&") {
        if (onlyLeaf(operand, "id") == nullptr) {
            error(n->line, "'&' can only be applied to a variable");
            return ERROR_TYPE;
        }
        if (t.pointer) {
            error(n->line, "pointers to pointers are not supported");
            return ERROR_TYPE;
        }
        return makeType(t.base, true);
    }
    if (op == "!" && isScalar(t)) return INT_TYPE;
    if (op != "!" && isNumeric(t)) return promoted(t);

    error(n->line, "invalid operand to unary '" + op + "' (" + typeName(t) + ")");
    return ERROR_TYPE;
}

// Primary -> id CallTail | int_const | float_const | char_const | string_lit | ( Expr )
Type Semantic::primary(const Node* n) {
    const Node* first = n->child(0);
    const std::string& kind = first->symbol;

    if (kind == "int_const") return INT_TYPE;
    if (kind == "char_const") return makeType(BaseType::Char);
    if (kind == "string_lit") return makeType(BaseType::Char, true);
    if (kind == "float_const") {
        const char last = first->token.value.back();
        return makeType(last == 'f' || last == 'F' ? BaseType::Float : BaseType::Double);
    }
    if (kind == "(") return expression(n->child(1));

    // id CallTail
    const std::string& name = first->token.value;
    const Node* tail = n->child(1);
    if (!tail->derivesEmpty()) return call(first, tail);

    const Symbol* s = symbols.lookup(name);
    if (s == nullptr) {
        error(first->line, "'" + name + "' was not declared");
        return ERROR_TYPE;
    }
    if (s->isFunction) {
        error(first->line, "function '" + name + "' used as a value (missing parentheses?)");
        return ERROR_TYPE;
    }
    return s->type;
}

// CallTail -> ( ArgList )
Type Semantic::call(const Node* id, const Node* callTail) {
    const std::string& name = id->token.value;
    const std::vector<const Node*> args = arguments(callTail->child(1));

    std::vector<Type> types;
    for (const Node* a : args) types.push_back(expression(a));

    const Symbol* s = symbols.lookup(name);
    if (s == nullptr) {
        error(id->line, "function '" + name + "' was not declared");
        return ERROR_TYPE;
    }
    if (!s->isFunction) {
        error(id->line, "'" + name + "' is not a function");
        return ERROR_TYPE;
    }

    const Symbol fn = *s;
    if (types.size() != fn.params.size()) {
        error(id->line, "function '" + name + "' expects " + std::to_string(fn.params.size()) +
                        " argument(s), got " + std::to_string(types.size()));
        return fn.type;
    }
    for (std::size_t i = 0; i < types.size(); ++i) {
        checkAssign(fn.params[i], types[i], id->line,
                    "argument " + std::to_string(i + 1) + " of '" + name + "'");
    }
    return fn.type;
}

// Regla de compatibilidad para =, inicializaciones, argumentos y return.
// Entre números se permite la conversión implícita como en C; solo cuando
// un flotante pasa a entero (se pierde la parte decimal) queda un warning.
void Semantic::checkAssign(Type target, Type value, int line, const std::string& what) {
    if (isError(target) || isError(value)) return;

    if (!value.pointer && value.base == BaseType::Void) {
        error(line, what + ": a void value cannot be used");
        return;
    }
    if (target.pointer || value.pointer) {
        if (!sameType(target, value)) {
            error(line, what + ": incompatible types, cannot convert " + typeName(value) + " to " + typeName(target));
        }
        return;
    }
    if (isFloating(value) && isIntegral(target)) {
        warning(line, what + ": implicit conversion from " + typeName(value) + " to " + typeName(target) +
                      " discards the fractional part");
    }
}
