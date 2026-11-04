#!/bin/sh
# Compila il solver in WebAssembly per l'analisi di uno slot dentro il browser (la finestra della coda):
# scrive sito/static/analisi.wasm e, accanto, l'impronta dei sorgenti (analisi.wasm.versione). Il sito usa il
# modulo solo se l'impronta corrisponde ai sorgenti del solver in uso; altrimenti chiede l'analisi al server.
# Va rieseguito (e committato il risultato) quando i sorgenti del solver cambiano.
#
# Serve wasi-sdk (https://github.com/WebAssembly/wasi-sdk): la variabile WASI_SDK deve puntare alla sua cartella.
set -e
cd "$(dirname "$0")/.."
[ -n "$WASI_SDK" ] || { echo "Imposta WASI_SDK alla cartella di wasi-sdk" >&2; exit 1; }
SORGENTI=$(python3 -c "
import re
b = re.search(r'add_executable\(\s*\S+(.*?)\)', open('CMakeLists.txt').read(), re.S).group(1)
print(' '.join(f for f in b.split() if f.endswith('.c')))")
"$WASI_SDK/bin/clang" -O2 -std=gnu11 -w -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_PROCESS_CLOCKS \
    -lwasi-emulated-signal -lwasi-emulated-process-clocks $SORGENTI -o sito/static/analisi.wasm
python3 -c "
import sys; sys.path.insert(0, '.')
from sito import solver
print(solver.impronta_sorgenti())" > sito/static/analisi.wasm.versione
echo "sito/static/analisi.wasm ($(wc -c < sito/static/analisi.wasm) byte), versione $(cat sito/static/analisi.wasm.versione)"
