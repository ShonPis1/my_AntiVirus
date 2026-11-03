#!/usr/bin/env bash
# Builds the scanner with sanitizers and checks it against generated files.
# Run from the repository root:  ./tests/run_tests.sh
set -u
cd "$(dirname "$0")/.."

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
CC=${CC:-gcc}
$CC -Wall -Wextra -g -fsanitize=address,undefined -o "$WORK/antivirus" AntiVirus_Magshimim/prog.c || exit 1

# signature with a zero byte inside (a C string function would stop at it)
SIG="$WORK/sig.bin"
printf 'ABC\0DEF' > "$SIG"

DIR="$WORK/scan"
mkdir "$DIR"
fill() { head -c "$1" /dev/zero | tr '\0' 'x'; }   # $1 bytes of filler

{ cat "$SIG"; fill 993; }                  > "$DIR/start.bin"
{ fill 496; cat "$SIG"; fill 497; }        > "$DIR/middle_of_the_file.bin"
{ fill 993; cat "$SIG"; }                  > "$DIR/end.bin"
fill 1000                                  > "$DIR/clean.bin"
{ fill 400; printf 'ABC'; fill 597; }      > "$DIR/prefix_only.bin"   # only the part before the zero byte
: > "$DIR/empty.bin"

fails=0
check() {  # check <scan option> <file name> <expected text>
	local out
	out=$(echo "$1" | "$WORK/antivirus" "$DIR" "$SIG" 2>&1) || { echo "FAIL: scanner crashed"; fails=$((fails+1)); return; }
	if echo "$out" | grep -qF "/$2 $3"; then echo "ok:   option $1  $2 -> $3"
	else echo "FAIL: option $1  $2 expected '$3'"; fails=$((fails+1)); fi
}

# normal scan: the whole file is checked
check 0 start.bin                "Infected!"
check 0 middle_of_the_file.bin   "Infected!"
check 0 end.bin                  "Infected!"
check 0 clean.bin                "Clean!"
check 0 prefix_only.bin          "Clean!"
check 0 empty.bin                "Clean!"

# quick scan: only the first and the last 20%
check 1 start.bin                "Infected! (first 20%)"
check 1 end.bin                  "Infected! (last 20%)"
check 1 middle_of_the_file.bin   "Clean!"
check 1 prefix_only.bin          "Clean!"

# the log file must not be scanned
if echo 0 | "$WORK/antivirus" "$DIR" "$SIG" | grep -qE "myAntiVirusLog.txt (Clean|Infected)"; then
	echo "FAIL: the log file was scanned"; fails=$((fails+1))
fi

[ "$fails" -eq 0 ] && echo "All tests passed" || { echo "$fails test(s) failed"; exit 1; }
