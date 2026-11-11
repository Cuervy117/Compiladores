#!/usr/bin/env bash
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project="$(dirname "$here")"
root="$(cd "$project/../../../../.." && pwd)"
src="$project/src"
out="$root/docs"

em++ -std=c++17 -O2 -lembind -I"$src" \
  -sALLOW_MEMORY_GROWTH=1 -sMODULARIZE=1 -sEXPORT_NAME=createLexerModule -sENVIRONMENT=web \
  -o "$out/lexer.js" \
  "$src/lexer.cpp" "$src/Token.cpp" "$src/TokenTable.cpp" "$here/wasm_bindings.cpp"
