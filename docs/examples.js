// Programas de prueba que aparecen en la pagina (salen de examples/ del proyecto)
var EXAMPLES = [
  {
    file: "01_declaration.c",
    name: "The example from the assignment",
    description: "One declaration with an initializer.",
    kind: "ok",
    expected: "Parsing Success! / SDT Verified!",
    code: "int x = 10;\n"
  },
  {
    file: "13_demo_success.c",
    name: "Demo 1: everything right",
    description: "A function, a call and printf. Both phases pass.",
    kind: "ok",
    expected: "Parsing Success! / SDT Verified!",
    code: "int square(int n) {\n    return n * n;\n}\n\nint total = square(4);\nprintf(\"total = %d\\n\", total);\n"
  },
  {
    file: "14_demo_semantic.c",
    name: "Demo 2: valid syntax, wrong meaning",
    description: "Same program, but square() gets a string and %d gets a float.",
    kind: "bad",
    expected: "Parsing Success! / SDT error...",
    code: "int square(int n) {\n    return n * n;\n}\n\nfloat total = square(\"4\");\nprintf(\"total = %d\\n\", total);\n"
  },
  {
    file: "15_demo_syntax.c",
    name: "Demo 3: syntax error",
    description: "A missing operand after *. The parser stops and the SDT is not evaluated.",
    kind: "bad",
    expected: "Parsing error... / SDT error...",
    code: "int square(int n) {\n    return n * ;\n}\n\nint total = square(4);\nprintf(\"total = %d\\n\", total);\n"
  },
  {
    file: "16_demo_warning.c",
    name: "Demo 4: warning only",
    description: "A float stored in an int: allowed in C, but the fractional part is lost.",
    kind: "warn",
    expected: "Parsing Success! / SDT Verified! + warning",
    code: "float price = 19.99;\nint pesos = price * 18.5;\nprint(pesos);\n"
  },
  {
    file: "ejemplo1.c",
    name: "Lexer stage example",
    description: "The reading example of the first deliverable.",
    kind: "ok",
    expected: "Parsing Success! / SDT Verified!",
    code: "printf(\"This is an example\");\nint a = 10;\n"
  },
  {
    file: "05_lexical_error.c",
    name: "Lexical error",
    description: "A character the lexer does not know.",
    kind: "bad",
    expected: "Parsing error... / SDT error...",
    code: "int x = 10 @ 2;\n"
  },
  {
    file: "03_missing_semicolon.c",
    name: "Missing semicolon",
    description: "The error is reported on the line where the ; should be.",
    kind: "bad",
    expected: "Parsing error... / SDT error...",
    code: "int a = 10\nint b = 20;\n"
  },
  {
    file: "06_undeclared.c",
    name: "Undeclared and redeclared names",
    description: "Two symbol table errors.",
    kind: "bad",
    expected: "Parsing Success! / SDT error...",
    code: "int x = 10;\ny = x + 1;\nint x = 3;\n"
  },
  {
    file: "07_types.c",
    name: "Type errors",
    description: "Incompatible initialization, % with a float, printf mismatch and a non-variable on the left of =.",
    kind: "bad",
    expected: "Parsing Success! / SDT error...",
    code: "int n = \"hola\";\nfloat f = 2.5;\nint r = f % 2;\nprintf(\"%d %s\\n\", f);\n5 = n;\n"
  },
  {
    file: "08_functions_and_loops.c",
    name: "Functions, return and break",
    description: "return/break in the wrong place and a wrong number of arguments.",
    kind: "bad",
    expected: "Parsing Success! / SDT error...",
    code: "void hello() {\n    print(\"hola\");\n    return 1;\n}\n\nint sum(int a, int b) {\n    return;\n}\n\nbreak;\nint v = sum(1);\nfloat w = hello();\nreturn 0;\n"
  },
  {
    file: "09_scopes.c",
    name: "Scopes",
    description: "A block hides a global variable; the for variable dies with the loop.",
    kind: "bad",
    expected: "Parsing Success! / SDT error...",
    code: "int x = 1;\n{\n    float x = 2.5;\n    x = x * 2;\n}\nfor (int i = 0; i < 3; i += 1) {\n    x += i;\n}\ni = 5;\n"
  },
  {
    file: "12_printf_scanf.c",
    name: "printf and scanf formats",
    description: "Addresses, specifier types and unknown specifiers.",
    kind: "bad",
    expected: "Parsing Success! / SDT error...",
    code: "int n;\nfloat f;\nscanf(\"%d\", n);\nscanf(\"%f\", &n);\nscanf(\"%lf\", &f);\nprintf(\"%d%%\\n\", n);\nprintf(\"%q\\n\", n);\nprintf(n);\n"
  },
  {
    file: "11_dangling_else.c",
    name: "Dangling else",
    description: "The else goes with the nearest if (the LL(1) conflict we resolved).",
    kind: "ok",
    expected: "Parsing Success! / SDT Verified!",
    code: "int a = 1, b = 2;\nif (a > 0)\n    if (b > 0)\n        print(1);\n    else\n        print(2);\n"
  },
  {
    file: "02_program.c",
    name: "Complete program",
    description: "Functions, loops, if/else, printf, scanf and print together. The tree is big.",
    kind: "ok",
    expected: "Parsing Success! / SDT Verified!",
    code: "// Factorial y promedio: usa funciones, ciclos, if/else y printf/scanf\nint factorial(int n) {\n    int result = 1;\n    while (n > 1) {\n        result = result * n;\n        n -= 1;\n    }\n    return result;\n}\n\nfloat average(int a, int b, int c) {\n    return (a + b + c) / 3.0f;\n}\n\nint main() {\n    int i, total = 0;\n    float avg;\n    char grade = 'A';\n\n    for (i = 1; i <= 5; i += 1) {\n        if (i % 2 == 0)\n            continue;\n        total += factorial(i);\n    }\n\n    avg = average(total, 10, 20);\n    if (avg >= 90.0) {\n        grade = 'A';\n    } else if (avg >= 80.0) {\n        grade = 'B';\n    } else\n        grade = 'C';\n\n    printf(\"total = %d, promedio = %.2f, grado %c\\n\", total, avg, grade);\n    scanf(\"%d\", &i);\n    print(total);\n    return 0;\n}\n"
  }
];
