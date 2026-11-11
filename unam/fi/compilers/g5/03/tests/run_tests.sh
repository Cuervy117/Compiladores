#!/usr/bin/env bash
# Corre el parser con cada ejemplo de examples/parser y compara la salida
# contra tests/expected/<nombre>.txt
#   bash tests/run_tests.sh            compara
#   bash tests/run_tests.sh --update   regenera las salidas esperadas
set -u

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project="$(dirname "$here")"
parser="$project/build/parser"
update=0
[ "${1:-}" = "--update" ] && update=1

pass=0
fail=0
for src in "$project"/examples/parser/*.c; do
    name="$(basename "$src" .c)"
    expected="$here/expected/$name.txt"
    actual="$("$parser" "$src" 2>/dev/null | tr -d '\r')"

    if [ $update -eq 1 ]; then
        printf '%s\n' "$actual" > "$expected"
        echo "actualizado  $name"
        continue
    fi

    if [ "$actual" = "$(tr -d '\r' < "$expected")" ]; then
        echo "ok    $name"
        pass=$((pass + 1))
    else
        echo "FAIL  $name"
        diff <(printf '%s\n' "$actual") <(tr -d '\r' < "$expected")
        fail=$((fail + 1))
    fi
done

[ $update -eq 1 ] && exit 0
echo
echo "$pass ok, $fail con diferencias"
[ $fail -eq 0 ]
