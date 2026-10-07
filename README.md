# Lexical Analyzer (Lexer) — Compilers

|                |                                                                                                                                                                                                  |
| -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **University** | Universidad Nacional Autónoma de México (UNAM)                                                                                                                                                   |
| **Faculty**    | Facultad de Ingeniería (Faculty of Engineering)                                                                                                                                                  |
| **Course**     | Compilers, Group 5                                                                                                                                                                               |
| **Team**       | 2                                                                                                                                                                                                |
| **Members**    | Díaz Antúnez David - 424104230<br>Galicia Rodríguez Sofia - 424040127<br>López López Carlos Daniel - 321228631<br>López Morales Fernando Samuel - 321172974<br>Quezada Olivares Emir - 321148836 |
| **Due date**   | October 6th, 2026                                                                                                                                                                                |
| **Repository** | https://github.com/Cuervy117/Compiladores                                                                                                                                                        |

## Table of Contents

1. [Introduction and Objective](#1-introduction-and-objective)
2. [Chosen Approach](#2-chosen-approach)
3. [Architecture and Project Structure](#3-architecture-and-project-structure)
4. [Token Definitions](#4-token-definitions)
5. [Context-Free Grammar](#5-context-free-grammar)
6. [Implementation](#6-implementation)
7. [Tests and Results](#8-tests-and-results)
8. [References](#9-references)

---

## 1. Introduction and Objective

A lexical analyzer (lexer) is the first phase of a compiler: it reads the source code as a sequence of characters and groups it into meaningful units called **tokens** (keywords, identifiers, operators, constants, and punctuation).

The goal of this project is to build a lexer that receives a string or a file, scans it, classifies each token into its category, and prints the list of categories found along with the total number of tokens. The project meets the minimal requirements of the assignment: it recognizes the basic tokens (including the keyword `print`), processes the reading examples from the assignment, and is accompanied by a proposal for a context-free grammar (CFG) with left factoring.

## 2. Chosen Approach

For the implementation of this lexer, we use the C++ programming language along with the *cctype* library, which provides useful functions such as `isalpha()`, `isdigit()`, and `isspace()` for character classification.

## 3. Architecture and Project Structure

```text
Compiladores/

├── docs
├── .gitignore
├── README.md
├── unam/fi/compilers/g5/03
    ├── examples
        ├── crlf.c
        ├── ejemplo1.c
        ├──  example_03.c
    ├── src
        ├── Lexer.hpp
        ├── Token.cpp
        ├── Token.hpp
        ├── TokenTable.cpp
        ├── TokenTable.hpp
        ├── lexer.cpp
        ├── main.cpp
    ├── wasm
        ├── build_wasm.sh
        ├── wasm_buildings.cpp
```

## 4. Token Definitions

The lexer classifies each token into one of the following categories (`enum class TokenType`):

| TokenType     | What it recognizes                       | Examples                                                                                                                               |
| ------------- | ---------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| `Keyword`     | Reserved words                           | int, float, double, char, void, if, else, while, for, do, return, break, continue, print, printf, scanf, struct, const, static, sizeof |
| `Identifier`  | Variable names                           | a, x                                                                                                                                   |
| `Operator`    | Operators                                | =, ==, !=, <=, >=, &&, +=, -=, *=, /=, %=, <, >, +, -, *, /, %, !, &                                                                   |
| `Constant`    | Numbers and text literals                | 10, "This is an example"                                                                                                               |
| `Punctuation` | Delimiters                               | (, ), {, }, [, ], ;, ,:                                                                                                                              |
| `EndOfFile`   | Marks the end of the input               | —                                                                                                                                      |
| `Unknown`     | Invalid character (reported as an error) | @                                                                                                                                      |

### Counting rules:

* `EndOfFile` and `Unknown` are **not** counted in the total number of tokens.
* A complete text literal, including its quotation marks, counts as **one** `Constant` token.
* Whitespace and line breaks between tokens are ignored.

## 5. Context-Free Grammar

The grammar models the reading example `printf("text");`. The start symbol is `S`
(the statement) and the terminals are the literal characters of the language.

```bnf
S  -> printf ( " T " A ) ;
A  -> , I A | ε
T  -> E T | ε
E  -> F | C
F  -> %d | %s | %f | %c | %%
C  -> L | M | O | SP | K
I  -> L I'
I' -> L I' | O I' | _ I' | ε
L  -> a | b | c | ... | z
M  -> A | B | C | ... | Z
O  -> 0 | 1 | 2 | ... | 9
SP -> ' '                     
K  -> + | - | ' | / | > | < | ! | : | . | , | # | @ | ? | $ | &
```

* `T` is the format string as a whole: a (possibly empty) sequence of `E`.
* `E` is either a format specifier `F` or a plain character `C`.
* `A` is the argument list after the closing quote: empty, or `,identifier` repeated.
* `I` / `I'` recognize identifiers of the form `[a-z][a-z0-9_]*`.

### Design notes

* **No left recursion.** `T -> E T` and `A -> , I A` are *right* recursive, so a
  top-down (LL) parser can process them without backtracking.
* **`%` belongs only to `F`.** The character `%` is deliberately excluded from
  `K`; otherwise `C` and `F` would both derive `%d`, making the grammar ambiguous
  (`FIRST(C) ∩ FIRST(F) = { % }`) and breaking LL(1).
* **`"` is not part of the string body.** Keeping the quote out of `K` preserves
  the string delimiters, so an embedded `"` cannot close the literal early.
* **`SP` covers spaces.** Without a space rule the grammar could not derive
  `printf("This is an example");`, which is Test 1 in this report.
* **`\n` is an escape**, not a format specifier, so it is treated as a plain
  character instead of being listed inside `F`.

### Example derivation (leftmost)

Derivation of `printf("%d",x);`:

```text
S
=> printf ( " T " A ) ;          (S)
=> printf ( " E T " A ) ;        (T -> E T)
=> printf ( " F T " A ) ;        (E -> F)
=> printf ( " %d T " A ) ;       (F -> %d)
=> printf ( " %d " A ) ;         (T -> ε)
=> printf ( " %d " , I A ) ;     (A -> , I A)
=> printf ( " %d " , L I' A ) ;  (I -> L I')
=> printf ( " %d " , x I' A ) ;  (L -> x)
=> printf ( " %d " , x A ) ;     (I' -> ε)
=> printf ( " %d " , x ) ;       (A -> ε)
```

## 6. Implementation
### 6.1 Scanning flow

1. The whole input is read.
2. The scanner advances character by character, skipping whitespace.
3. Based on the current character, it decides which kind of token is being formed (letter → identifier or keyword, digit → numeric constant, quotation mark → text literal, symbol → operator or punctuation).
4. The complete token is consumed and classified with `TokenType`.
5. If the character does not match any category, an `Unknown` token is emitted.
6. When the input ends, `EndOfFile` is emitted, and the categories and the total are printed.

### 6.2 Main functions

| Function / class | What it does |
| ---------------- | ------------ |
| enum class TokenType | Defines the token categories (keyword, identifier, operator, constant, punctuation, end of file, unknown). |
| struct Token | Stores a token's category and its original text (lexeme). |
| class Lexer | Scans the source code and splits it into tokens, keeping a cursor on the current character. |
| main() | Reads a line of code, runs the lexer until end of file, and prints the token sequence and total count. |
| namespace TokenTable | Groups the lookup tables and functions for the supported lexemes. |
| TokenTable::lookupKeyword() | Returns TokenType::Keyword if the lexeme is a reserved word, or nothing otherwise. |
| TokenTable::isOperator() | Checks whether a full lexeme (e.g. "==" or "+") is a valid operator. |
| TokenTable::isOperatorChar() | Checks whether a character can be part of an operator. |
| TokenTable::isPunctuation() | Checks whether a character is a punctuation symbol such as "(" or ";". |
| TokenTable::totalLexemes() | Returns the total number of distinct lexemes supported (keywords, operators and punctuation). |
| operatorChars() | Builds the set of characters that can form an operator, derived from the operator table. |
| punctuationChars() | Returns the string of supported punctuation characters. |

## 7. Tests and Results

### Test 1: `printf("This is an example");`

**Input:**

```c
printf("This is an example");
```

**Program output:**

```text
--
```

### Test 2: `int a = 10;`

**Input:**

```c
int a = 10;
```

**Program output:**

```text
--
```

## 8. References

* Class notes and theoretical sessions of the Compilers course, Facultad de Ingeniería, UNAM.


