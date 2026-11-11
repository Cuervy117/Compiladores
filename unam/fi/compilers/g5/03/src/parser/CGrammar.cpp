#include "CGrammar.hpp"

namespace {


//  Gramática para LL(1): 
const char* const GRAMMAR = R"(
Program     -> ItemList
ItemList    -> Item ItemList | ε
Item        -> Type id ItemTail | Stmt
ItemTail    -> ( ParamList ) Block | VarTail
Type        -> int | float | double | char | void

ParamList   -> Param ParamMore | ε
ParamMore   -> , Param ParamMore | ε
Param       -> Type id

VarTail     -> Init VarMore ;
VarMore     -> , id Init VarMore | ε
Init        -> = Expr | ε

Block       -> { BlockItems }
BlockItems  -> BlockItem BlockItems | ε
BlockItem   -> Type id VarTail | Stmt

Stmt        -> ExprStmt | Block | IfStmt | WhileStmt | DoStmt | ForStmt
             | ReturnStmt | IoStmt | break ; | continue ;
ExprStmt    -> Expr ; | ;
IfStmt      -> if ( Expr ) Stmt ElsePart
ElsePart    -> else Stmt | ε
WhileStmt   -> while ( Expr ) Stmt
DoStmt      -> do Stmt while ( Expr ) ;
ForStmt     -> for ( ForInit OptExpr ; OptExpr ) Stmt
ForInit     -> Type id VarTail | ExprStmt
OptExpr     -> Expr | ε
ReturnStmt  -> return OptExpr ;
IoStmt      -> IoFunc ( ArgList ) ;
IoFunc      -> printf | print | scanf
ArgList     -> Expr ArgMore | ε
ArgMore     -> , Expr ArgMore | ε

Expr        -> OrExpr AssignTail
AssignTail  -> AssignOp Expr | ε
AssignOp    -> = | += | -= | *= | /= | %=
OrExpr      -> AndExpr OrTail
OrTail      -> || AndExpr OrTail | ε
AndExpr     -> EqExpr AndTail
AndTail     -> && EqExpr AndTail | ε
EqExpr      -> RelExpr EqTail
EqTail      -> EqOp RelExpr EqTail | ε
EqOp        -> == | !=
RelExpr     -> AddExpr RelTail
RelTail     -> RelOp AddExpr RelTail | ε
RelOp       -> < | > | <= | >=
AddExpr     -> MulExpr AddTail
AddTail     -> AddOp MulExpr AddTail | ε
AddOp       -> + | -
MulExpr     -> Unary MulTail
MulTail     -> MulOp Unary MulTail | ε
MulOp       -> * | / | %
Unary       -> UnaryOp Unary | Primary
UnaryOp     -> - | + | ! | &
Primary     -> id CallTail | int_const | float_const | char_const | string_lit | ( Expr )
CallTail    -> ( ArgList ) | ε
)";

}

const char* cGrammarText() {
    return GRAMMAR;
}

const Grammar& cGrammar() {
    static const Grammar grammar(GRAMMAR);
    return grammar;
}
