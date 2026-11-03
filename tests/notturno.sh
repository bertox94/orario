#!/bin/bash
# Test notturni: prova il solver su molti dati casuali, con numeri di classi diversi, e raccoglie un riepilogo.
#
# Uso: tests/notturno.sh [ore] [semi_per_giro]
#   ore            durata massima in ore (default 8): non parte un nuovo giro se il tempo e' finito
#   semi_per_giro  quanti semi per ogni giro e numero di classi (default 20, cioe' 40 casi con i ritardi)
#
# Ogni giro usa semi nuovi (il primo seme riparte da dove si era arrivati, ricordato in tests/notturni/prossimo_seme),
# prova 4, 6, 8, 10 e 12 classi e scrive in tests/notturni/<data>/classi_<N>/ i risultati completi.
# Le stampe delle marce indietro (molto grandi) occupano al massimo 3 GB per prova e si tengono solo per l'ultima
# prova: di tutte le altre restano sempre dati.txt, risultato.txt e orario.txt di ogni caso.
# Il riepilogo di tutta la notte e' in tests/notturni/<data>/riepilogo.txt.
#
# Da lanciare con: nohup tests/notturno.sh 8 > /dev/null 2>&1 &   oppure da cron: 0 23 * * * /percorso/tests/notturno.sh 8
set -u
ORE=${1:-8}
SEMI=${2:-20}
AQUI=$(cd "$(dirname "$0")" && pwd)
BASE="$AQUI/notturni"
DATA=$(date +%Y-%m-%d_%H%M)
OUT="$BASE/$DATA"
mkdir -p "$OUT"
PROSSIMO="$BASE/prossimo_seme"
FINE=$(( $(date +%s) + ORE * 3600 ))
RIEP="$OUT/riepilogo.txt"
echo "Test notturni $DATA, durata massima ${ORE} h, $SEMI semi per giro" > "$RIEP"
fallite=0

while [ "$(date +%s)" -lt "$FINE" ]; do
    DA=$(cat "$PROSSIMO" 2>/dev/null || echo 1)
    echo $(( DA + SEMI )) > "$PROSSIMO"
    for CLASSI in 4 6 8 10 12; do
        [ "$(date +%s)" -ge "$FINE" ] && break
        CART="$OUT/classi_${CLASSI}_da_${DA}"
        python3 "$AQUI/casuali.py" --da "$DA" --quanti "$SEMI" --classi "$CLASSI" --out "$CART" --marce-gb 3 \
            > "$CART.log" 2>&1
        esito=$?
        [ $esito -ne 0 ] && fallite=$((fallite + 1))
        {
            echo "== classi $CLASSI, semi $DA..$((DA + SEMI - 1)): $( [ $esito -eq 0 ] && echo 'tutti finiti' || echo 'ci sono casi non finiti o errori')"
            tail -n 12 "$CART.log"
        } >> "$RIEP"
        # le stampe delle marce indietro pesano molto: si tengono solo quelle dell'ultima prova
        find "$OUT" -type d -name marce_indietro -not -path "$CART/*" -exec rm -rf {} + 2>/dev/null
    done
done
echo "Fine: $(date '+%Y-%m-%d %H:%M'). Prove con casi non finiti o errori: $fallite" >> "$RIEP"
