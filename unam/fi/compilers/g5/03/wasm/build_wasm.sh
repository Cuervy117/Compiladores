#!/usr/bin/env bash
# Compila lexer + parser + SDT a WebAssembly y deja compiler.js / compiler.wasm en docs/.
# Necesita Emscripten activo en la terminal (source ~/emsdk/emsdk_env.sh).
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project="$(dirname "$here")"
root="$(cd "$project/../../../../.." && pwd)"
src="$project/src"
out="$root/docs"

em++ -std=c++17 -O2 -lembind -I"$src" \
  -sALLOW_MEMORY_GROWTH=1 -sMODULARIZE=1 -sEXPORT_NAME=createCompilerModule -sENVIRONMENT=web \
  -o "$out/compiler.js" \
  "$src/lexer.cpp" "$src/Token.cpp" "$src/TokenTable.cpp" \
  "$src/parser/Grammar.cpp" "$src/parser/CGrammar.cpp" "$src/parser/ParseTree.cpp" "$src/parser/Parser.cpp" \
  "$src/semantic/SymbolTable.cpp" "$src/semantic/Semantic.cpp" \
  "$src/app/Analysis.cpp" \
  "$here/wasm_bindings.cpp"

echo "Listo: $out/compiler.js y $out/compiler.wasm"
